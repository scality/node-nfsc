'use strict';

var nfsc = require('../../index');
var config = require('../config.json');
var assert = require('assert');
var crypto = require('crypto');
var async = require('async');

describe('NFSv3 client test', () => {

    var test_dir = 'foo_' + crypto.randomBytes(8).toString('hex');
    let mnt;

    it('should create a new instance', done => {
        mnt = new nfsc.V3(config);
        assert.strictEqual(typeof(mnt), 'object');
        done();
    });

    it('should mount the filesystem', done => {
        mnt.mount((err, fh) => {
            assert.strictEqual(err, null);
            assert.strictEqual(typeof(fh), 'object');
            mnt.unmount(done);
        });
    });

    it(`should create a directory ${test_dir}`, done => {
        mnt.mount((err, root) => {
            assert.strictEqual(err, null);
            assert.strictEqual(typeof(root), 'object');
            mnt.mkdir(root, test_dir, { mode: 0o755 }, (err, wcc) => {
                assert.strictEqual(err, null);
                mnt.unmount(done);
            });
        });
    });

    it('should create, stat, write, read and remove a file', done => {
        let dir;
        async.waterfall([
            function mount(next) {
                mnt.mount((err, root) => {
                    assert.strictEqual(err, null);
                    assert.strictEqual(typeof(root), 'object');
                    next(null, root);
                });
            },
            function lookupDir(root, next) {
                mnt.lookup(root, test_dir, (err, dir, obj_attrs, dir_attrs) => {
                    assert.strictEqual(err, null);
                    assert.strictEqual(typeof(dir), 'object');
                    next(null, dir);
                });
            },
            function createFiles(dir, next) {
                var filename = 'bar_' + crypto.randomBytes(8).toString('hex');
                mnt.create(dir, filename, mnt.CREATE_GUARDED, { mode: 0o644 },
                           (err, object, obj_attrs, dir_wcc) => {
                               assert.strictEqual(err, null);
                               assert.strictEqual(typeof(object), 'object');
                               next(null, dir, object, filename, obj_attrs);
                           });
            },
            function getattrFile(dir, object, filename, obj_attrs, next) {
                mnt.getattr(object, (err, second_obj_attrs) => {
                    assert.strictEqual(err, null);
                    assert.deepStrictEqual(obj_attrs, second_obj_attrs);
                    next(null, object, dir, filename);
                });
            },
            function writeFile(object, dir, filename, next) {
                var buffer = crypto.randomBytes(4096);
                mnt.write(object, buffer.length, 0, mnt.WRITE_UNSTABLE, buffer,
                          (err, commited, count, verf, wcc) => {
                              assert.strictEqual(err, null);
                              assert.strictEqual(count, buffer.length);
                              assert.strictEqual(typeof(verf), 'object');
                              next(null, object, dir, filename, buffer);
                          });
            },
            function readFile(object, dir, filename, buffer, next) {
                mnt.getattr(object, (err, obj_attrs) => {
                    assert.strictEqual(err, null);
                    assert.strictEqual(typeof(obj_attrs), 'object');
                    assert.strictEqual(obj_attrs.size, buffer.length);
                    mnt.read(object, obj_attrs.size, 0, (err, eof, buf, attrs) => {
                        assert.strictEqual(err, null);
                        assert.deepStrictEqual(buffer, buf);
                        /* eof is not guaranted in this case with vfs fsal */
                        /* assert.strictEqual(eof, true); */
                        next(null, dir, filename);
                    });
                });
            },
            function removeFile(dir, filename, next) {
                mnt.remove(dir, filename, (err, dir_wcc) => {
                    assert.strictEqual(err, null);
                    next(null, dir, filename);
                });
            },
            function relookupFile(dir, filename, next) {
                mnt.lookup(dir, filename, (err, fh, obj_attrs, dir_attrs) => {
                    assert.strictEqual(err, 'NFS3ERR_NOENT');
                    next(null);
                });
            },
            function unmount(next) {
                mnt.unmount(next);
            }
        ], err => {
            assert.strictEqual(err, null);
            done();
        });
    });

});
