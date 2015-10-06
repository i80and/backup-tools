'use strict'

const aws = require('aws-sdk')
const fs = require('fs')

const S3 = function(options) {
    this.s3 = new aws.S3(options)
}

S3.prototype.uploadArchive = function(bucket, path, description, hints) {
    const stream = fs.createReadStream(path)

    const params = {
        'Bucket': bucket,
        'Body': stream,
        'Key': description,
        'ACL': 'private'
    }

    return new Promise((resolve, reject) => {
        this.s3.putObject(params, (err, data) => {
            if(err !== null) {
                return reject(err, null)
            }

            return resolve(null, description)
        })
    })
}

S3.prototype.getInventory = function(bucket) {
    return new Promise((resolve, reject) => {
        this.s3.listObjects({'Bucket': bucket}, (err, data) => {
            if(err !== null) {
                return reject(err, null)
            }

            const results = []
            data.Contents.forEach(function(archive) {
                results.push({
                    'id': archive.Key,
                    'description': archive.Key
                })
            })
            return resolve(null, results)
        })
    })
}

S3.prototype.getArchive = function(bucket, archiveID) {
    return new Promise((resolve, reject) => {
        this.s3.getObject({'Bucket': bucket, 'Key': archiveID}, (err, data) => {
            if(err !== null) {
                return reject(err, null)
            }

            return resolve(null, {
                'key': archiveID,
                'description': archiveID,
                'body': data.Body
            })
        })
    })
}

S3.prototype.removeArchives = function(bucket, keys) {
    const keyObjects = keys.map((key) => { return {'Key': key} })

    return new Promise((resolve, reject) => {
        this.s3.deleteObjects({'Bucket': bucket,
                               'Delete': {'Objects': keyObjects}}, (err, data) => {
            if(err !== null) {
                return resolve(err, null)
            }

            return resolve(null, data)
        })
    })
}

exports.S3 = S3
