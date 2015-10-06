'use strict'

const aws = require('aws-sdk')
const fs = require('fs')

const S3 = function(options) {
    this.s3 = new aws.S3(options)
}

S3.prototype.uploadArchive = function(bucket, path, description, callback, hints) {
    // XXX we should be doing multi-part uploads to avoid needing to read the whole shebang in
    const data = fs.readFileSync(path)

    const params = {
        'Bucket': bucket,
        'Body': data,
        'Key': description,
        'ACL': 'private'
    }

    this.s3.putObject(params, (err, data) => {
        if(err !== null) {
            callback(err)
        }
        else {
            callback(null, description)
        }
    })
}

S3.prototype.getInventory = function(bucket, callback) {
    this.s3.listObjects({'Bucket': bucket}, (err, data) => {
        if(err !== null) {
            callback(err, null)
        } else {
            const results = []
            data.Contents.forEach(function(archive) {
                results.push({
                    'id': archive.Key,
                    'description': archive.Key
                })
            })
            callback(null, results)
        }
    })
}

S3.prototype.getArchive = function(bucket, archiveID, callback) {
    this.s3.getObject({'Bucket': bucket, 'Key': archiveID}, (err, data) => {
        if(err !== null) {
            callback(err, null)
        }
        else {
            callback(null, {
                'key': archiveID,
                'description': archiveID,
                'body': data.Body
            })
        }
    })
}

S3.prototype.removeArchive = function(bucket, key, callback) {
    this.s3.deleteObject({'Bucket': bucket, 'Key': key}, (err, data) => {
        callback(err, data)
    })
}

exports.S3 = S3
