'use strict'

var aws = require('aws-sdk')
var fs = require('fs')

var Glacier = function(options) {
    this.glacier = new aws.Glacier(options)
}

Glacier.prototype.uploadArchive = function(vault, path, description, callback) {
    // XXX we should be doing multi-part uploads to avoid needing to read the whole shebang in
    var data = fs.readFileSync(path)

    var params = {
        vaultName: vault,
        body: data,
        archiveDescription: description
    }

    this.glacier.uploadArchive(params, function(err, data) {
        if(err !== null) {
            callback(err)
        }
        else {
            callback(null, data.archiveId)
        }
    })
}

Glacier.prototype.getInventory = function(vault, callback) {
    var glacier = this

    throw "Not implemented"

    var params = {}
    params.vaultName = vault
    params.jobParameters = {
        'Type': 'inventory-retrieval',
        'Description': 'Retrieving inventory of ' + vault
    }
    glacier.glacier.initiateJob(params, function(err, data) {
        if(err !== null) {
            throw err
        }

        // XXX Should use SNS instead of polling
        // Every ten minutes, check for a result
        var intervalID = setInterval(function() {
            glacier.glacier.describeJob({'vaultName': vault, 'jobId': data.jobId}, function(err, data) {
                if(err !== null) {
                    throw err
                }

                if(data.StatusCode === "Failed") {
                    throw data.StatusMessage
                }

                if(data.Completed) {
                    clearInterval(intervalID)
                    glacier.glacier.getJobOutput({'vaultName': vault, 'jobId': data.jobId}, function(err, data) {
                        var inventory = JSON.parse(data.body)
                        var results = []
                        inventory.ArchiveList.forEach(function(archive) {
                            results.push({
                                'id': archive.ArchiveId,
                                'description': archive.ArchiveDescription
                            })
                        })

                        callback(results)
                    })
                }
            })
        }, 1000*60*10)
    })
}

Glacier.prototype.getArchive = function(vault, archive, callback) {
    throw "Not implemented"
}

Glacier.prototype.removeArchive = function(vault, archiveId, callback) {
    this.glacier.deleteArchive({'vaultName': vault, 'archiveId': archiveId}, function(err, data) {
        callback(err, data)
    })
}

exports.Glacier = Glacier
