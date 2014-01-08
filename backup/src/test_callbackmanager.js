'use strict'

var callbackmanager = require('./callbackmanager')

exports.testUnboundedBarrier = function(test) {
    test.expect(4)

    var barrier = new callbackmanager.Barrier()
    test.equal(barrier.maxConcurrency, Infinity, 'Default maxConcurrency value is wrong')

    var task1 = function(next) {
        setTimeout(function() {
            task1.done = true
            next()
        }, 50)
    }
    task1.done = false

    var task2 = function(next) {
        setTimeout(function() {
            task2.done = true

            // Even though task1 is started first, task2 should finish first
            test.ok(!task1.done, 'task2 finished after task1')

            next()
        }, 10)
    }
    task2.done = false

    barrier.doTask(task1)
    barrier.doTask(task2)

    barrier.wait(function() {
        test.ok(task1.done, 'task1 not completed')
        test.ok(task2.done, 'task2 not completed')

        test.done()
    })
}

exports.testBoundedBarrier = function(test) {
    test.expect(8)

    // Only allow a single task to run at a time
    var barrier = new callbackmanager.Barrier(2)
    test.equal(barrier.maxConcurrency, 2, 'Assigned maxConcurrency value is wrong')

    // Task1 and task2 fire first.  Task1 finishes, and Task3 is started.  Task3 finishes.  Task2 finishes.

    var task1
    var task2
    var task3

    task1 = function(next) {
        setTimeout(function() {
            task1.done = true
            next()
        }, 10)
    }
    task1.done = false

    task2 = function(next) {
        setTimeout(function() {
            task2.done = true
            test.ok(task1.done)
            test.ok(task3.done)
            next()
        }, 50)
    }
    task2.done = false

    task3 = function(next) {
        setTimeout(function() {
            task3.done = true

            test.ok(task1.done)
            test.ok(!task2.done)

            next()
        }, 1)
    }
    task3.done = false

    barrier.doTask(task1)
    barrier.doTask(task2)
    barrier.doTask(task3)

    barrier.wait(function() {
        test.ok(task1.done, 'task1 not completed')
        test.ok(task2.done, 'task2 not completed')
        test.ok(task3.done, 'task3 not completed')

        test.done()
    })
}

exports.testSerializer = function(test) {
    test.expect(3)

    var task1 = function(next) {
        setTimeout(function() {
            task1.done = true
            next()
        }, 50)
    }
    task1.done = false

    var task2 = function(next) {
        setTimeout(function() {
            task2.done = true

            test.ok(task1.done, 'task2 finished before task1')

            next()
        }, 10)
    }
    task2.done = false

    callbackmanager.serialize([task1, task2], function() {
        test.ok(task1.done, 'task1 not completed')
        test.ok(task2.done, 'task2 not completed')

        test.done()
    })
}

exports.testNoCallback = function(test) {
    var task = function(next) {
        next()
    }

    callbackmanager.serialize([])

    test.done()
}
