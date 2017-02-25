'use strict';
/*
 * Copyright 2017 Scality
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @authors:
 *    Guillaume Gimenez <ggim@scality.com>
 */
let impl;
try {
    impl = require('../build/Release/node-nfsc');
} catch (err) {
    impl = require('../build/Debug/node-nfsc');
}

const defaultProtocol = 'udp';
const defaultUid = process.geteuid();
const defaultGid = process.getegid();
const defaultAuthenticationMethod = 'unix';
const defaultTimeout = 25;

function int53(i) {
    if (i < Number.MIN_SAFE_INTEGER || i > Number.MAX_SAFE_INTEGER)
        throw '53-bit overflow';
    return i;
}

class V3 {

    /**
     * Construct a new NFSv3 Client instance with the given parameters
     *
     * @param {object} options contains the export parameters
     * @param {string} options.host host name or IP address of the NFSv3 server
     * @param {string} options.exportPath path of the export on the NFSc3 server
     * @param {string} options.protocol may be 'udp' or 'tcp'
     * @param {integer} options.uid User ID to use on requests to the server
     * @param {integer} options.gid Group ID to use on requests to the server
     * @param {string} options.authenticationMethod 'none', 'unix', 'krb5',
     *                                              'krb5i' or 'krb5p'
     * @param {string} options.timeout timeout in seconds for network operations
     */
    constructor(opts) {
        const options = opts ? opts : {};
        const host = options.host;
        const exportPath = options.exportPath;
        const protocol = options.protocol === undefined
            ? defaultProtocol : options.protocol;
        const uid = options.uid === undefined
            ? defaultUid : options.uid;
        const gid = options.gid === undefined
            ? defaultGid : options.gid;
        const authenticationMethod = options.authenticationMethod === undefined
            ? defaultAuthenticationMethod : options.authenticationMethod;
        const timeout = options.timeout === undefined
            ? defaultTimeout : options.timeout;
        this.client = new impl.Client(host, exportPath, protocol,
                                      uid, gid, authenticationMethod,
                                      timeout);

        /* unix modes */
        this.MODE_IRWXU = 0o700;
        this.MODE_IRUSR = 0o400;
        this.MODE_IWUSR = 0o200;
        this.MODE_IXUSR = 0o100;

        this.MODE_IRWXG = 0o070;
        this.MODE_IRGRP = 0o040;
        this.MODE_IWGRP = 0o020;
        this.MODE_IXGRP = 0o010;

        this.MODE_IRWXO = 0o007;
        this.MODE_IROTH = 0o004;
        this.MODE_IWOTH = 0o002;
        this.MODE_IXOTH = 0o001;

        /* constants from the RFC 1813 */
        /* ACCESS3 */
        this.ACCESS_READ    = 0x0001;
        this.ACCESS_LOOKUP  = 0x0002;
        this.ACCESS_MODIFY  = 0x0004;
        this.ACCESS_EXTEND  = 0x0008;
        this.ACCESS_DELETE  = 0x0010;
        this.ACCESS_EXECUTE = 0x0020;
    }

    /**
     * Procedure MNT maps a pathname on the server to a file
     * handle.  The pathname is an ASCII string that describes a
     * directory on the server. If the call is successful
     * (MNT3_OK), the server returns an NFS version 3 protocol
     * file handle and a vector of RPC authentication flavors
     * that are supported with the client's use of the file
     * handle (or any file handles derived from it).  The
     * authentication flavors are defined in Section 7.2 and
     * section 9 of [RFC1057].
     *
     * @param {function} calllback(err: string, root_file_handle: Buffer);
     *                   On success, err is null. Continue execution with
     *                   the file handle of the root inode of the export.
     *                   On error, err contains information about the error.
     * @returns {undefined}
     */
    mount(callback) {
        this.client.mount3(callback);
    }

    /**
     * Procedure LOOKUP searches a directory for a specific name
     * and returns the file handle for the corresponding file
     * system object.
     * 
     * @param {Buffer} dir The file handle for the directory to search.
     * @param {string} name The filename to be searched for.
     * @param {function} callback(err: string, object: Buffer,
     *                            obj_attributes: Object,
     *                            dir_attributes: Object);
     *                   On success, err is null. Continue execution with
     *                   the 'object' handle, its attributes and the attributes
     *                   of the directory as well.
     *                   On error, err contains information about the error.
     * @returns {undefined}
     */
    lookup(dir, name, callback) {
        this.client.lookup3(dir, name, callback);
    }

    /**
     * Procedure GETATTR retrieves the attributes for a specified
     * file system object. The object is identified by the file
     * handle that the server returned as part of the response
     * from a LOOKUP, CREATE, MKDIR, SYMLINK, MKNOD, or
     * READDIRPLUS procedure (or from the MOUNT procedure)
     *
     * @param {Buffer} object The file handle of an object whose attributes are
     *                        to be retrieved.
     * @param {function} callback(err: string, obj_attributes: Object);
     *                   On success, err is null. Continue execution with
     *                   attributes for the object.
     *                   On error, err contains information about the error.     
     * @returns {undefined}
     */
    getattr(object, callback) {
        this.client.getattr3(object, callback);
    }

    /**
     * Procedure READDIR retrieves a variable number of entries,
     * in sequence, from a directory and returns the name and
     * file identifier for each, with information to allow the
     * client to request additional directory entries in a
     * subsequent READDIR request.
     *
     * @param {Buffer} dir The file handle for the directory to be read.
     * @param {Object} options
     * @param {Buffer} options.cookie
     *                     This should be set to null in the first subsequent
     *                     requests, it should be a cookie as returned by the
     *                     server.
     * @param {Buffer} options.cookieverf
     *                     This should be set to null in the first request to
     *                     read the directory. On subsequent requests, it
     *                     should be a cookieverf as returned by the server.
     *                     The cookieverf must match that returned by the
     *                     READDIR in which the cookie was acquired.
     * @param {integer} count
     *                     The maximum size of the READDIR3resok structure, in
     *                     bytes.  The size must include all XDR overhead. The
     *                     server is free to return less than count bytes of
     *                     data.
     * @param {function} callback(err: string, dir_attributes: Object,
     *                            eof: bool, [ { cookie: Buffer,
     *                                           fileid: Buffer,
     *                                           name: string}, ... ]);
     * @returns {undefined}
     */
    readdir(dir, options, callback) {
        const opts = options ? options : {};
        opts.cookie = opts.cookie || null;
        opts.cookieverf = opts.cookieverf || null;
        /* like a Linux NFS Client */
        opts.count = opts.count || 32688;
        this.client.readdir3(dir, opts.cookie, opts.cookieverf, opts.count, callback);
    }

    /**
     * Procedure READDIRPLUS retrieves a variable number of
     * entries from a file system directory and returns complete
     * information about each along with information to allow the
     * client to request additional directory entries in a
     * subsequent READDIRPLUS.  READDIRPLUS differs from READDIR
     * only in the amount of information returned for each
     * entry.  In READDIR, each entry returns the filename and
     * the fileid.  In READDIRPLUS, each entry returns the name,
     * the fileid, attributes (including the fileid), and file
     * handle.
     *
     * @param {Buffer} dir The file handle for the directory to be read.
     * @param {Object} options
     * @param {Buffer} options.cookie
     *                     This should be set to null in the first subsequent
     *                     requests, it should be a cookie as returned by the
     *                     server.
     * @param {Buffer} options.cookieverf
     *                     This should be set to null in the first request to
     *                     read the directory. On subsequent requests, it
     *                     should be a cookieverf as returned by the server.
     *                     The cookieverf must match that returned by the
     *                     READDIR in which the cookie was acquired.
     * @param {integer} dircount
     *                     The maximum number of bytes of directory information
     *                     returned. This number should not include the size of
     *                     the attributes and file handle portions of the result.
     * @param {integer} maxcount
     *                     The maximum size of the READDIR3resok structure, in
     *                     bytes.  The size must include all XDR overhead. The
     *                     server is free to return less than count bytes of
     *                     data.
     * @param {function} callback(err: string,
     *                            dir_attributes: Object,
     *                            cookieverf: Buffer,
     *                            eof: bool, [ { handle: Buffer,
     *                                           attributes: Object,
     *                                           cookie: Buffer,
     *                                           fileid: Buffer,
     *                                           name: string}, ... ]);
     * @returns {undefined}
     */
    readdirplus(dir, options, callback) {
        const opts = options ? options : {};
        opts.cookie = opts.cookie || null;
        opts.cookieverf = opts.cookieverf || null;
        /* like a Linux NFS Client */
        opts.dircount = opts.dircount || 8172;
        opts.maxcount = opts.maxcount || 32688;
        this.client.readdirplus3(dir, opts.cookie, opts.cookieverf,
                                 opts.dircount, opts.maxcount, callback);
    }

    /**
     * Procedure ACCESS determines the access rights that a user,
     * as identified by the credentials in the request, has with
     * respect to a file system object. The client encodes the
     * set of permissions that are to be checked in a bit mask.
     * The server checks the permissions encoded in the bit mask.
     * A status of NFS3_OK is returned along with a bit mask
     * encoded with the permissions that the client is allowed.
     *
     * The results of this procedure are necessarily advisory in
     * nature.  That is, a return status of NFS3_OK and the
     * appropriate bit set in the bit mask does not imply that
     * such access will be allowed to the file system object in
     * the future, as access rights can be revoked by the server
     * at any time.
     *
     * @param {Buffer} object The file handle for the file system object to
     *                        which access is to be checked.
     * @param {integer} access A bit mask of access permissions to check.
     * @param {function} callback(err: string, access: integer,
     *                            obj_attributes: Object);
     * @return {undefined}
     */
    access(object, access, callback) {
        this.client.access3(object, access, callback);
    }

    /**
     * Procedure NULL does not do any work. It is made available to
     * allow server response testing and timing.
     *
     * @param {function} callback(err)
     *                   On success, err is null.
     *                   On error, err contains information about the error.     
     * @return {undefined}
     */
    null(callback) {
        this.client.null3(callback);
    }

    /**
     * Procedure READ reads data from a file.
     *
     * @param {Buffer} object The file handle of the file from which data is to
     *                        be read. This must identify a file system object
     *                        of type 'NF3REG'.
     * @param {integer} count The number of bytes of data that are to be read.
     *                        If count is 0, the READ will succeed and return 0
     *                        bytes of data, subject to access permissions
     *                        checking. count must be less than or equal to the
     *                        value of the rtmax field in the FSINFO reply
     *                        structure for the file system that contains file.
     *                        If greater, the server may return only rtmax
     *                        bytes, resulting in a short read.
     * @param {integer} offset
     *                        The position within the file at which the read is
     *                        to begin.  An offset of 0 means to read data
     *                        starting at the beginning of the file. If offset
     *                        is greater than or equal to the size of the file,
     *                        the status, NFS3_OK, is returned with count set to
     *                        0 and eof set to TRUE, subject to access
     *                        permissions checking.
     * @param {function} callback(err: string, eof: bool,
     *                            buf: Buffer,
     *                            obj_attributes: Object);
     * @returns {undefined}
     */
    read(object, count, offset, callback) {
        this.client.read3(object, int53(count), int53(offset), callback);
    }
}

module.exports = { V3 };
