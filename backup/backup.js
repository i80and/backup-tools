'use strict'

const fs = require('fs')
const s3 = require('./src/s3')
const callbackmanager = require('./src/callbackmanager')

const BackupSystem = function(siteID, vaultName, options) {
    if(options === undefined) { options = {} }
    if(options.awsOptions === undefined) { options.awsOptions = {} }

    this.siteID = siteID

    const creationOptions = {apiVersion: '2014-01-01'}
    for(let key in options.awsOptions) {
        creationOptions[key] = options.awsOptions[key]
    }

    this.storage = new s3.S3(creationOptions)
    this.vaultName = vaultName
}

BackupSystem.prototype.backup = function(path, expiry, callback) {
    const now = new Date()

    // If the object has already expired somehow, that's a no-op
    if(now >= expiry) {
        callback(null)
    }

    const description = this.siteID + ' ' + now.toISOString() + ' ' + expiry.toISOString()
    this.storage.uploadArchive(this.vaultName, path, description, callback, {'Expires': expiry})
}

BackupSystem.prototype.list = function(callback) {
    let inventory

    // Get the Glacier vault inventory
    const getInventoryNoodle = (next) => {
        this.storage.getInventory(this.vaultName, function(err, data) {
            if(err !== null) {
                callback(err, null)
                return
            }

            inventory = data
            next()
        })
    }

    // Using the inventory, remove any expired archives
    const listNoodle = function(next) {
        const results = []

        inventory.forEach(function(archive) {
            const descriptionSegments = archive.description.split(' ')
            const expiryDate = new Date(Date.parse(descriptionSegments[descriptionSegments.length - 1]))
            const archiveDate = new Date(Date.parse(descriptionSegments[descriptionSegments.length - 2]))

            // XXX Should we deal with invalid dates here, or pass the buck (status quo)
            results.push({
                'id': archive.id,
                'description': archive.description,
                'expiry': expiryDate,
                'date': archiveDate
            })
        })

        callback(null, results)
    }

    callbackmanager.serialize([
        getInventoryNoodle,
        listNoodle
    ])
}

BackupSystem.prototype.prune = function(callback) {
    this.list((err, archives) => {
        // Only allow 5 deletion tasks to execute at once, and wait for all of them to finish before
        // hitting the callback
        const barrier = new callbackmanager.Barrier(5)

        archives.forEach(function(archive) {
            const shouldPrune = isNaN(archive.date) || isNaN(archive.expiry) || (archive.expiry < (new Date()))
            if(shouldPrune) {
                barrier.doTask(function(next) {
                    this.storage.removeArchive(this.vaultName, archive.id, next)
                })
            }
        })

        barrier.wait(callback)
    })
}

BackupSystem.prototype.restore = function(archiveID, callback) {
    this.storage.getArchive(this.vaultName, archiveID, (err, archive) => {
        if(err !== null) {
            callback(err)
            return
        }

        fs.writeFile(archive.key, archive.body, () => {
            callback(null)
        })
    })
}

function main(argv) {
    const options = {}
    options.awsOptions = {}
    options.awsOptions.accessKeyId = argv.awsKeyID
    options.awsOptions.secretAccessKey = argv.awsSecretKey
    options.awsOptions.region = argv.awsRegion

    const backupSystem = new BackupSystem(argv.name, argv.vault, options)

    const backupNoodle = (next) => {
        if(argv.backup) {
            // Expire in either a year or a week
            const expiry = new Date()
            if(argv.longTermBackup) {
                expiry.setFullYear(expiry.getFullYear() + 1)
            }
            else {
                expiry.setDate(expiry.getDate() + 7)
            }

            backupSystem.backup(argv.backup, expiry, (err, archiveID) => {
                if(err !== null) {
                    console.error('Error backing up: ' + err)
                }
                else {
                    console.log('Backup created: Archive ' + archiveID)
                }

                next()
            })
        }
        else {
            next()
        }
    }

    const pruneNoodle = (next) => {
        if(argv.prune) {
            backupSystem.prune(() => {
                next()
            })
        }
        else {
            next()
        }
    }

    const listBackupsNoodle = (next) => {
        if(argv.list) {
            backupSystem.list((err, archives) => {
                if(err !== null) {
                    throw err
                }

                archives.forEach((archive) => {
                    console.log(archive.id + ':\t' + archive.date)
                })

                next()
            })
        }
        else {
            next()
        }
    }

    const restoreNoodle = (next) => {
        if(argv.restore) {
            backupSystem.restore(argv.restore, (err, data) => {
                if(err !== null) {
                    throw err
                }

                next()
            })
        }
        else {
            next()
        }
    }

    callbackmanager.serialize([
        backupNoodle,
        pruneNoodle,
        listBackupsNoodle,
        restoreNoodle
    ])
}

main(require('yargs')
    .usage('Usage: $0')
    .demand(['name', 'awsKeyID', 'awsSecretKey', 'vault'])
    .describe('name', 'The name of the backup site; this will be prefixed to the date.')
    .describe('vault', 'The name of the Glacier vault to use.')
    .describe('awsKeyID', 'Your public AWS key.')
    .describe('awsSecretKey', 'Your AWS secret key.')
    .describe('backup', 'The archive to upload to S3.')
    .boolean('longTermBackup')
    .default('longTermBackup', false)
    .describe('longTermBackup', 'Keep the backup for a year instead of a week.')
    .describe('awsRegion', 'The AWS region to use.')
    .default('awsRegion', 'us-east-1')
    .boolean('prune')
    .describe('prune', 'Prune old backups.  May take several hours to complete.')
    .boolean('list')
    .describe('list', 'List all backups currently in the inventory.')
    .describe('restore', 'Restore a given backup ID.')
    .check(function(args) {
        if(args.list) { return true }
        if(args.hasOwnProperty('restore')) { return true }
        if(args.hasOwnProperty('backup')) { return true }
        if(args.prune) { return true }

        throw 'No action provided: either --backup, --restore, --prune, or --list must be provided.'
    })
    .argv
)
