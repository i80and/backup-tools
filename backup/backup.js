'use strict'

const fs = require('fs')
const process = require('process')
const s3 = require('./src/s3')

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

BackupSystem.prototype.backup = function(path, expiry) {
    const now = new Date()

    // If the object has already expired somehow, that's a no-op
    if(now >= expiry) {
        return new Promise((resolve, reject) => { resolve(null) })
    }

    const description = this.siteID + ' ' + now.toISOString() + ' ' + expiry.toISOString()
    return this.storage.uploadArchive(this.vaultName, path, description, {'Expires': expiry})
}

BackupSystem.prototype.list = function() {
    // Get the S3 vault inventory
    return this.storage.getInventory(this.vaultName).then((inventory) => {
        // Using the inventory, remove any expired archives
        const results = []
        for(let archive of inventory) {
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
        }

        return results
    })
}

BackupSystem.prototype.prune = function() {
    return this.list().then((archives) => {
        const toPrune = archives.filter((archive) => {
            return isNaN(archive.date) || isNaN(archive.expiry) || (archive.expiry < (new Date()))
        }).map((archive) => archive.id)

        return this.storage.removeArchives(this.vaultName, toPrune)
    })
}

BackupSystem.prototype.restore = function(archiveID) {
    const outStream = fs.createWriteStream(archiveID)
    outStream.on('error', (err) => {
        console.error('Error writing file ' + archiveID)
    })

    return this.storage.getArchive(this.vaultName, archiveID, outStream).then((archive) => {
        return new Promise((resolve, reject) => {
            console.log('Finished restoring ' + archive.key)
            return resolve()
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

    new Promise((resolve, reject) => {
        // Backup
        if(!argv.backup) { return resolve() }

        // Expire in either a year or a week
        const expiry = new Date()
        if(argv.longTermBackup) {
            expiry.setFullYear(expiry.getFullYear() + 1)
        }
        else {
            expiry.setDate(expiry.getDate() + 7)
        }

        return backupSystem.backup(argv.backup, expiry).then((archiveID) => {
            console.log('Backup created: Archive ' + archiveID)
        })
    }).then(() => {
        // Prune
        if(!argv.prune) { return }

        return backupSystem.prune()
    }).then(() => {
        // List backups
        if(!argv.list) { return }

        return backupSystem.list().then((archives) => {
            for(let archive of archives) {
                console.log(archive.id + ':\t' + archive.date)
            }
        })
    }).then(() => {
        // Restore
        if(!argv.restore) { return }

        return backupSystem.restore(argv.restore)
    }).catch((err) => {
        console.error(err)
        process.exit(1)
    })
}

main(require('yargs')
    .usage('Usage: $0')
    .demand(['name', 'awsKeyID', 'awsSecretKey', 'vault'])
    .describe('name', 'The name of the backup site; this will be prefixed to the date.')
    .describe('vault', 'The name of the S3 bucket to use.')
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
