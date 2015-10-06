'use strict'

// Callback manager that can wait for a series of concurrent asynchronous tasks to end
const Barrier = function(maxConcurrency) {
    this.pending = 0
    this.maxConcurrency = (maxConcurrency === undefined)? Infinity : maxConcurrency
    this.waiting = []
    this.callback = null

    this.doNext = () => {
        this.pending -= 1

        // If we have waiting tasks, fire one up to replace the just-completed task
        if(this.waiting.length > 0) {
            this.pending += 1
            return this.waiting.shift()(this.doNext)
        }

        // If we're done, fire the barrier callback
        if(this.pending <= 0) {
            if(this.callback) {
                this.callback()
            }
        }
    }
}

Barrier.prototype.doTask = function(task) {
    if(this.pending >= this.maxConcurrency) {
        // Too many tasks currently executing; put it in the queue
        this.waiting.push(task)
        return
    }

    this.pending += 1
    task(this.doNext)
}

Barrier.prototype.wait = function(callback) {
    if(!callback) { return }

    // If we're done, go into our callback now
    if(this.pending === 0 && this.waiting.length === 0) {
        callback()
    }

    // Otherwise, wait for the tasks to finish
    this.callback = callback
}

// Execute an array of asynchronous tasks in sequential order
const serialize = function(tasks, callback) {
    const barrier = new Barrier(1)
    tasks.forEach(function(task) {
        barrier.doTask(task)
    })

    barrier.wait(callback)
}

exports.serialize = serialize
exports.Barrier = Barrier
