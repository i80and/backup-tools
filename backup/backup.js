'use strict'

var fs = require('fs')
var glacier = require('./src/glacier')
var s3 = require('./src/s3')
var callbackmanager = require('./src/callbackmanager')

var BackupSystem = function(siteID, vaultName, options) {
    if(options === undefined) options = {}
    if(options.glacierOptions === undefined) options.glacierOptions = {}

    this.siteID = siteID

    var creationOptions = {apiVersion: '2014-01-01'}
    for(var key in options.glacierOptions) {
        creationOptions[key] = options.glacierOptions[key]
    }

    this.storage = new s3.S3(creationOptions)
    this.vaultName = vaultName
}

BackupSystem.prototype.backup = function(path, expiry, callback) {
    var now = new Date()

    // If the object has already expired somehow, that's a no-op
    if(now >= expiry) {
        callback(null)
    }

    var description = this.siteID + ' ' + now.toISOString() + ' ' + expiry.toISOString()
    this.storage.uploadArchive(this.vaultName, path, description, callback, {'Expires': expiry})
}

BackupSystem.prototype.list = function(callback) {
    var backupSystem = this
    var inventory

    // Get the Glacier vault inventory
    var getInventoryNoodle = function(next) {
        backupSystem.storage.getInventory(backupSystem.vaultName, function(err, data) {
            if(err !== null) {
                callback(err, null)
                return
            }

            inventory = data
            next()
        })
    }

    // Using the inventory, remove any expired archives
    var listNoodle = function(next) {
        var results = []

        inventory.forEach(function(archive) {
            var descriptionSegments = archive.description.split(' ')
            var expiryDate = new Date(Date.parse(descriptionSegments[descriptionSegments.length - 1]))
            var archiveDate = new Date(Date.parse(descriptionSegments[descriptionSegments.length - 2]))

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
    var backupSystem = this

    backupSystem.list(function(err, archives) {
        // Only allow 5 deletion tasks to execute at once, and wait for all of them to finish before
        // hitting the callback
        var barrier = new callbackmanager.Barrier(5)

        archives.forEach(function(archive) {
            var shouldPrune = isNaN(archive.date) || isNaN(archive.expiry) || (archive.expiry < (new Date()))
            if(shouldPrune) {
                barrier.doTask(function(next) {
                    backupSystem.storage.removeArchive(backupSystem.vaultName, archive.id, next)
                })
            }
        })

        barrier.wait(callback)
    })
}

BackupSystem.prototype.restore = function(archiveID, callback) {
    var backupSystem = this

    backupSystem.storage.getArchive(this.vaultName, archiveID, function(err, archive) {
        if(err !== null) {
            callback(err)
            return
        }

        fs.writeFile(archive.key, archive.body, function() {
            callback(null)
        })
    })
}

var main = function(argv) {
    var options = {}
    options.glacierOptions = {}
    options.glacierOptions.accessKeyId = argv.awsKeyID
    options.glacierOptions.secretAccessKey = argv.awsSecretKey
    options.glacierOptions.region = argv.awsRegion

    var backupSystem = new BackupSystem(argv.name, argv.vault, options)

    var backupNoodle = function(next) {
        if(argv.backup) {
            // Expire in either a year or a week
            var expiry = new Date()
            if(argv.longTermBackup) {
                expiry.setFullYear(expiry.getFullYear() + 1)
            }
            else {
                expiry.setDate(expiry.getDate() + 7)
            }

            backupSystem.backup(argv.backup, expiry, function(err, archiveID) {
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

    var pruneNoodle = function(next) {
        if(argv.prune) {
            backupSystem.prune(function() {
                next()
            })
        }
        else {
            next()
        }
    }

    var listBackupsNoodle = function(next) {
        if(argv.list) {
            backupSystem.list(function(err, archives) {
                if(err !== null) {
                    throw err
                }

                archives.forEach(function(archive) {
                    console.log(archive.id + ':\t' + archive.date)
                })

                next()
            })
        }
        else {
            next()
        }
    }

    var restoreNoodle = function(next) {
        if(argv.restore) {
            backupSystem.restore(argv.restore, function(err, data) {
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

main(require('optimist')
    .usage('Usage: $0')
    .demand(['name', 'awsKeyID', 'awsSecretKey', 'vault'])
    .describe('name', 'The name of the backup site; this will be prefixed to the date.')
    .describe('vault', 'The name of the Glacier vault to use.')
    .describe('awsKeyID', 'Your public AWS key.')
    .describe('awsSecretKey', 'Your AWS secret key.')
    .describe('backup', 'The archive to upload to glacier.')
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
        if(args.list) return true
        if(args.hasOwnProperty('restore')) return true
        if(args.hasOwnProperty('backup')) return true
        if(args.prune) return true

        throw 'No action provided: either --backup, --restore, --prune, or --list must be provided.'
    })
    .argv
)
