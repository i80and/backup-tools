'use strict'

var aws = require('aws-sdk')
var fs = require('fs')

var S3 = function(options) {
    this.s3 = new aws.S3(options)
}

S3.prototype.uploadArchive = function(bucket, path, description, callback, hints) {
    // XXX we should be doing multi-part uploads to avoid needing to read the whole shebang in
    var data = fs.readFileSync(path)

    var params = {
        'Bucket': bucket,
        'Body': data,
        'Key': description,
        'ACL': 'private'
    }

    this.s3.putObject(params, function(err, data) {
        if(err !== null) {
            callback(err)
        }
        else {
            callback(null, description)
        }
    })
}

S3.prototype.getInventory = function(bucket, callback) {
    this.s3.listObjects({'Bucket': bucket}, function(err, data) {
        if(err !== null) {
            callback(err, null)
        }
        else {
            var results = []
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
    this.s3.getObject({'Bucket': bucket, 'Key': archiveID}, function(err, data) {
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
    this.s3.deleteObject({'Bucket': bucket, 'Key': key}, function(err, data) {
        callback(err, data)
    })
}

exports.S3 = S3
