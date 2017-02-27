'use strict';

var nfsc = require('../../index');
var config = require('../config.json');
var assert = require('assert');
var crypto = require('crypto');

describe('NFSv3 client test', () => {

    let mnt;
    let root;
    let test_dir = crypto.randomBytes(4).toString('hex');

    it('should create a new instance', done => {
        mnt = new nfsc.V3(config);
            assert.strictEqual(typeof(mnt), 'object');
        done();
    });
    
    it('should mount the filesystem', done => {
        mnt.mount((err, fh) => {
            assert.strictEqual(err, null);
            assert.strictEqual(typeof(fh), 'object');
            root = fh;
            done();
        });
    });
});
