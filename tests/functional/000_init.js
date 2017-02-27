'use strict';

let nfsc;
let impl;
let configuration;

describe('module', () => {
    it('should load the nan module without error', done => {
        try {
            impl = require('../../build/Release/node-nfsc');
        } catch (err) {
            impl = require('../../build/Debug/node-nfsc');
        }
        done();
    });
    it('should load the js module without error', done => {
        nfsc = require('../../index');
        done();
    });
    it('should load the test configuration without error', done => {
        configuration = require('../config.json');
        done();
    });
});
