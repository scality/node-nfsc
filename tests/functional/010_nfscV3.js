'use strict';

var nfsc = require('../../index');
var config = require('../config.json');
var assert = require('assert');
var crypto = require('crypto');
var async = require('async');

var test_dir = 'foo_' + crypto.randomBytes(8).toString('hex');
let mnt;

describe('NFSv3 client base test', () => {


    it('should create a new instance', done => {
        mnt = new nfsc.V3(config);
        assert.strictEqual(typeof(mnt), 'object');
        done();
    });

    it('should mount the filesystem', done => {
        mnt.mount((err, fh) => {
            assert.strictEqual(err, null);
            assert.strictEqual(typeof(fh), 'object');
            mnt.unmount(err => {
                assert.strictEqual(err, null);
                done();
            });
        });
    });

    it(`should create a directory ${test_dir}`, done => {
        mnt.mount((err, root) => {
            assert.strictEqual(err, null);
            assert.strictEqual(typeof(root), 'object');
            mnt.mkdir(root, test_dir, { mode: 0o755 }, (err, wcc) => {
                assert.strictEqual(err, null);
                mnt.unmount(err => {
                    assert.strictEqual(err, null);
                    done();
            });
            });
        });
    });

});
/**
 * This is a convenience function that permit mocha tests to
 * take place in a waterfall. and then start the next test with
 * the result of the previous.
 *
 * Anti-pattern spotted: tests must be decoupled
 *
 * @param {string} msg
 * @param {function} callback(doneCallback: function)
 *     and     doneCallback(waterfallNextCallback: function:
 *                          ...argsForWaterfallNextCallback);
 */
function describeIt(msg, callback) {
    describe('Waterfall step', () => {
        it(msg, done => {
            callback(function () {
                done();
                if (arguments.length) {
                    const next = Array.from(arguments);
                    const args = next.splice(1);
                    next[0].apply(null, args);
                }
            });
        });
    });
}

let root_fh;

async.waterfall([
    (next) =>
        describeIt('should mount the filesystem', done =>{
            mnt.mount((err, root) => {
                assert.strictEqual(err, null);
                assert.strictEqual(typeof(root), 'object');
                root_fh = root;
                done(next, null, root);
            });
        }),
    (root, next) =>
        describeIt('should lookup the directory', done =>{
            mnt.lookup(root, test_dir, (err, dir, obj_attrs, dir_attrs) => {
                assert.strictEqual(err, null);
                assert.strictEqual(typeof(dir), 'object');
                done(next, null, dir);
            });
        }),
    (dir, next) =>
        describeIt('should create a file', done =>{
            var filename = 'bar_' + crypto.randomBytes(8).toString('hex');
            mnt.create(dir, filename, mnt.CREATE_GUARDED, { mode: 0o644 },
                       (err, object, obj_attrs, dir_wcc) => {
                           assert.strictEqual(err, null);
                           assert.strictEqual(typeof(object), 'object');
                           done(next, null, dir, object, filename, obj_attrs);
                       });
        }),
    (dir, object, filename, obj_attrs, next) =>
        describeIt('should getattr on file', done => {
            mnt.getattr(object, (err, second_obj_attrs) => {
                assert.strictEqual(err, null);
                assert.deepStrictEqual(obj_attrs, second_obj_attrs);
                done(next, null, object, dir, filename);
            });
        }),
    (object, dir, filename, next) =>
        describeIt('should write a file', done => {
            var buffer = crypto.randomBytes(4096);
            mnt.write(object, buffer.length, 0, mnt.WRITE_UNSTABLE, buffer,
                      (err, commited, count, verf, wcc) => {
                          assert.strictEqual(err, null);
                          assert.strictEqual(count, buffer.length);
                          assert.strictEqual(typeof(verf), 'object');
                          done(next, null, object, dir, filename, buffer);
                      });
        }),
    (object, dir, filename, buffer, next) =>
        describeIt('should getattr on file', done => {
            mnt.getattr(object, (err, obj_attrs) => {
                assert.strictEqual(err, null);
                assert.strictEqual(typeof(obj_attrs), 'object');
                assert.strictEqual(obj_attrs.size, buffer.length);
                mnt.read(object, obj_attrs.size, 0, (err, eof, buf, attrs) => {
                    assert.strictEqual(err, null);
                    assert.deepStrictEqual(buffer, buf);
                    /* eof is not guaranted in this case with vfs fsal */
                    /* assert.strictEqual(eof, true); */
                    done(next, null, dir, filename);
                });
            });
        }),
    (dir, filename, next) =>
        describeIt('should remove the file', done => {
            mnt.remove(dir, filename, (err, dir_wcc) => {
                assert.strictEqual(err, null);
                done(next, null, dir, filename);
            });
        }),
    (dir, filename, next) =>
        describeIt('should ENOENT on file lookup', done => {
            mnt.lookup(dir, filename, (err, fh, obj_attrs, dir_attrs) => {
                assert.strictEqual(err, 'NFS3ERR_NOENT');
                done(next, null);
            });
        }),
    (next) =>
        describeIt('should remove the directory', done => {
            mnt.rmdir(root_fh, test_dir, (err, wcc) => {
                done(next, err);
            });
        }),
    (next) =>
        describeIt('should ENOENT on directory lookup', done => {
            mnt.lookup(root_fh, test_dir, (err, fh, obj_attrs, dir_attrs) => {
                assert.strictEqual(err, 'NFS3ERR_NOENT');
                done(next, null);
            });
        }),
    next =>
        describeIt('should create a named pipe', done => {
            var name = 'bar_' + crypto.randomBytes(8).toString('hex');
            mnt.mknod(root_fh, name, mnt.NF3FIFO, {attrs: {}},
                      (err, object, obj_attrs, dir_wcc) => {
                          assert.strictEqual(err, null);
                          assert.strictEqual(typeof(object), 'object');
                          assert.strictEqual(obj_attrs.type, mnt.NF3FIFO);
                          done(next, null);
                      });
        }),
    next =>
        describeIt('should create a socket', done => {
            var name = 'bar_' + crypto.randomBytes(8).toString('hex');
            mnt.mknod(root_fh, name, mnt.NF3SOCK, {attrs: {}},
                      (err, object, obj_attrs, dir_wcc) => {
                          assert.strictEqual(err, null);
                          assert.strictEqual(typeof(object), 'object');
                          assert.strictEqual(obj_attrs.type, mnt.NF3SOCK);
                          done(next, null);
                      });
        }),
    next =>
        describeIt('should create a char device', done => {
            var name = 'bar_' + crypto.randomBytes(8).toString('hex');
            mnt.mknod(root_fh, name, mnt.NF3CHR,
                      {
                          rdev: {
                              major: 10,
                              minor: 10
                          },
                          attrs: {}
                      },
                      (err, object, obj_attrs, dir_wcc) => {
                          assert.strictEqual(err, null);
                          assert.strictEqual(typeof(object), 'object');
                          assert.strictEqual(obj_attrs.type, mnt.NF3CHR);
                          assert.strictEqual(obj_attrs.rdev.major, 10);
                          assert.strictEqual(obj_attrs.rdev.minor, 10);
                          done(next, null);
                      });
        }),
    next =>
        describeIt('should create a block device', done => {
            var name = 'bar_' + crypto.randomBytes(8).toString('hex');
            mnt.mknod(root_fh, name, mnt.NF3BLK,
                      {
                          rdev: {
                              major: 10,
                              minor: 10
                          },
                          attrs: {}
                      },
                      (err, object, obj_attrs, dir_wcc) => {
                          assert.strictEqual(err, null);
                          assert.strictEqual(typeof(object), 'object');
                          assert.strictEqual(obj_attrs.type, mnt.NF3BLK);
                          assert.strictEqual(obj_attrs.rdev.major, 10);
                          assert.strictEqual(obj_attrs.rdev.minor, 10);
                          done(next, null);
                      });
        }),
    (next) =>
        describeIt('should unmount the filesystem', done => {
            mnt.unmount(err => {
                done(next, err);
            });
        })
], err =>
    describeIt('should complete the waterfall', done => {
        assert.strictEqual(err, null);
        done();
    }));
