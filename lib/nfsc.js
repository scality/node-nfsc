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

        /* WRITE3 stable_how */
        this.WRITE_UNSTABLE  = 0;
        this.WRITE_DATA_SYNC = 1;
        this.WRITE_FILE_SYNC = 2;

        /* CREATE3 createmode3 */
        this.CREATE_UNCHECKED = 0;
        this.CREATE_GUARDED   = 1;
        this.CREATE_EXCLUSIVE = 2;
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
      * Procedure UMNT removes the mount list entry for the
      * directory that was previously the subject of a MNT call
      * from this client.  AUTH_UNIX authentication or better is
      * required.
      *
      * @param {function} callback(err: string);
      */
    unmount(callback) {
        this.client.unmount3(callback);
    }

    /**
     * Procedure LOOKUP searches a directory for a specific name
     * and returns the file handle for the corresponding file
     * system object.
     * 
     * @param {Buffer} dir The file handle for the directory to search.
     * @param {string} name The filename to be searched for.
     * @param {function} callback(err: string, object: Buffer,
     *                            obj_attributes: Object || null,
     *                            dir_attributes: Object || null);
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
     * @param {function} callback(err: string, dir_attributes: Object || null,
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
     *                            dir_attributes: Object || null,
     *                            cookieverf: Buffer,
     *                            eof: bool, [ { handle: Buffer,
     *                                           attributes: Object || null,
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
     *                            post_obj_attr: Object || null);
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
     *                            obj_attributes: Object || null);
     * @returns {undefined}
     */
    read(object, count, offset, callback) {
        this.client.read3(object, int53(count), int53(offset), callback);
    }

    /**
      * Procedure WRITE writes data to a file.
      *
      * @param {Buffer} object The file handle for the file to which data is to
      *                        be written.  This must identify a file system
      *                        object of type, NF3REG.
      * @param {integer} count The number of bytes of data to be written. If
      *                        count is 0, the WRITE will succeed and return a
      *                        count of 0, barring errors due to permissions
      *                        checking. The size of data must be less than or
      *                        equal to the value of the wtmax field in the
      *                        FSINFO reply structure for the file system that
      *                        contains file. If greater, the server may write
      *                        only wtmax bytes, resulting in a short write.
      * @param {integer} offset
      *                        The position within the file at which the write
      *                        is to begin.  An offset of 0 means to write data
      *                        starting at the beginning of the file.
      * @param {stable_how} stable
      *                        If stable is FILE_SYNC, the server must commit
      *                        the data written plus all file system metadata to
      *                        stable storage before returning results. This
      *                        corresponds to the NFS version 2 protocol
      *                        semantics. Any other behavior constitutes a
      *                        protocol violation. If stable is DATA_SYNC, then
      *                        the server must commit all of the data to stable
      *                        storage and enough of the metadata to retrieve
      *                        the data before returning. The server implementor
      *                        is free to implement DATA_SYNC in the same
      *                        fashion as FILE_SYNC, but with a possible
      *                        performance drop. If stable is UNSTABLE, the
      *                        server is free to commit any part of the data and
      *                        the metadata to stable storage, including all or
      *                        none, before returning a reply to the client.
      *                        There is no guarantee whether or when any
      *                        uncommitted data will subsequently be committed
      *                        to stable storage. The only guarantees made by
      *                        the server are that it will not destroy any data
      *                        without changing the value of verf and that it
      *                        will not commit the data and metadata at a level
      *                        less than that requested by the client. See the
      *                        discussion on COMMIT n page 92 for more
      *                        information on if and when data is committed to
      *                        stable storage.
      * @param {Buffer} data   The data to be written to the file.
      * @param {function} callback(err: null,
      *                            commited: stable_how,
      *                            count: integer,
      *                            verf: Buffer,
      *                            attrs: { before: Object, after: Object});
      * @param {function} callback(err: string,
      *                            file_wcc: { before: Object, after: Object});
      *
      * @note verf
      *   This is a cookie that the client can use to determine
      *   whether the server has changed state between a call to
      *   WRITE and a subsequent call to either WRITE or COMMIT.
      *   This cookie must be consistent during a single instance
      *   of the NFS version 3 protocol service and must be
      *   unique between instances of the NFS version 3 protocol
      *   server, where uncommitted data may be lost.
      * @note file_wcc
      *   Weak cache consistency data for the file. For a client
      *   that requires only the post-write file attributes,
      *   these can be found in file_wcc.after. Even though the
      *   COMMIT failed, full wcc_data is returned to allow the
      *   client to determine whether the file changed on the
      *   server between calls to WRITE and COMMIT.
      *
      */
    write(object, count, offset, stable, data, callback) {
        this.client.write3(object, count, offset, stable, data, callback);
    }

    /**
      * Procedure COMMIT forces or flushes data to stable storage
      * that was previously written with a WRITE procedure call
      * with the stable field set to UNSTABLE.
      *
      * @param {Buffer} object The file handle for the file to which data is to
      *                        be flushed (committed). This must identify a file
      *                        system object of type, NF3REG.
      * @param {integer} count The number of bytes of data to flush. If count is
      *                        0, a flush from offset to the end of file is
      *                        done.
      * @param {integer} offset
      *                        The position within the file at which the flush
      *                        is to begin.  An offset of 0 means to flush data
      *                        starting at the beginning of the file.
      * @param {function} callback(err: null,
      *                            verf: Buffer,
      *                            attrs: { before: Object, after: Object});
      * @param {function} callback(err: string,
      *                            file_wcc: { before: Object, after: Object});
      */
    commit(object, count, offset, callback) {
        this.client.commit3(object, count, offset, callback);
    }

    /**
      * Procedure CREATE creates a regular file.
      *
      * @param {Buffer} dir The file handle for the directory in which the file
      *                     is to be created.
      * @param {string} name
      *                     The name that is to be associated with the created
      *                     file.
      * @param {createmode3} mode
      *                  One of UNCHECKED, GUARDED, and EXCLUSIVE. UNCHECKED
      *                  means that the file should be created without checking
      *                  for the existence of a duplicate file in the same
      *                  directory. In this case, how.obj_attributes is a sattr3
      *                  describing the initial attributes for the file. GUARDED
      *                  specifies that the server should check for the presence
      *                  of a duplicate file before performing the create and
      *                  should fail the request with NFS3ERR_EXIST if a
      *                  duplicate file exists. If the file does not exist, the
      *                  request is performed as described for UNCHECKED.
      *                  EXCLUSIVE specifies that the server is to follow
      *                  exclusive creation semantics, using the verifier to
      *                  ensure exclusive creation of the target. No attributes
      *                  may be provided in this case, since the server may use
      *                  the target file metadata to store the createverf3
      *                  verifier.
      * @param {Object|Buffer} arg
      *                  Depending on the mode, must be a 8 bytes buffer
      *                  containing the verifier or an Object compluing with
      *                  setattr requirement.
      * @param {funcion} callback(err: null,
      *                           object: Buffer || null,
      *                           obj_attrs: Object || null,
      *                           dir_wcc: Object);
      * @param {function} callback(err: string, dir_wcc: Object);
      *
      */
    create(dir, name, mode, arg, callback) {
        this.client.create3(dir, name, mode, arg, callback);
    }

    /**
      * Procedure REMOVE removes (deletes) an entry from a
      * directory. If the entry in the directory was the last
      * reference to the corresponding file system object, the
      * object may be destroyed.
      *
      * In general, REMOVE is intended to remove non-directory
      * file objects and RMDIR is to be used to remove
      * directories.  However, REMOVE can be used to remove
      * directories, subject to restrictions imposed by either the
      * client or server interfaces.  This had been a source of
      * confusion in the NFS version 2 protocol.
      *
      * @param {Buffer} dir
      *                   The file handle for the directory from which the entry
      *                   is to be removed.
      * @param {string} name
      *                   The name of the entry to be removed.
      * @param {function} callback(err: null,
      *                            dir_wcc: Object);
      * @param {function} callback(err: string,
      *                            dir_wcc: Object);
      */
    remove(dir, name, callback) {
        this.client.remove3(dir, name, callback);
    }

    /**
      * Procedure RMDIR removes (deletes) a subdirectory from a
      * directory. If the directory entry of the subdirectory is
      * the last reference to the subdirectory, the subdirectory
      * may be destroyed.
      *
      * @param {Buffer} dir
      *                   The file handle for the directory from which the
      *                   subdirectory is to be removed.
      * @param {string} name
      *                   The name of the subdirectory to be removed.
      * @param {function} callback(err: null,
      *                            dir_wcc: Object);
      * @param {function} callback(err: string,
      *                            dir_wcc: Object);
      */
    rmdir(dir, name, callback) {
        this.client.rmdir3(dir, name, callback);
    }

    /**
      * Procedure MKDIR creates a new subdirectory.
      *
      * @param {Buffer} dir
      *                  The file handle for the directory in which the
      *                  subdirectory is to be created.
      * @param {string} name
      *                  The name that is to be associated with the created
      *                  subdirectory.
      * @param {Object} attrs
      *                  The initial attributes for the subdirectory.
      * @param {funcion} callback(err: null,
      *                           object: Buffer || null,
      *                           obj_attrs: Object || null,
      *                           dir_wcc: Object);
      * @param {function} callback(err: string, dir_wcc: Object);
      *
      */
    mkdir(dir, name, arg, callback) {
        this.client.mkdir3(dir, name, arg, callback);
    }

    /**
      * Procedure SETATTR changes one or more of the attributes of
      * a file system object on the server. The new attributes are
      * specified by a sattr3 structure.
      *
      * @param {Buffer} object
      *                   The file handle for the object.
      * @param {Object} new_attributes
      *                   A sattr3 structure containing booleans and
      *                   enumerations describing the attributes to be set and
      *                   the new values for those attributes.
      * @param {Object} guard
      *                   A client may request that the server check that the
      *                   object is in an expected state before performing the
      *                   SETATTR operation. To do this, it sets the argument
      *                   an object and the client passes a time value in ctime
      *                   and ctime_nsec. If guard is set, the server
      *                   must compare the value of ctime to the
      *                   current ctime of the object. If the values are
      *                   different, the server must preserve the object
      *                   attributes and must return a status of
      *                   NFS3ERR_NOT_SYNC. If guard is null, the server
      *                   will not perform this check.
      * @param {function} callback(err: null,
      *                            wcc: Object);
      * @param {function} callback(err: string,
      *                            wcc: Object);
      * @note sattr3 object
      *  The known attributes for this object are:
      *   - atime, atime_nsec
      *   - mtime, mtime_nsec
      *   - size
      *   - mode
      *   - uid
      *   - gid
      *  If one of these attributes is left undefined, the corresponding
      *  attributes will remain unmodified on the server.
      */
    setattr(object, attrs, guard, callback) {
        this.client.setattr3(object, attrs, guard, callback);
    }

    /**
      * Procedure RENAME renames the file identified by from.name
      * in the directory, from.dir, to to.name in the di- rectory,
      * to.dir. The operation is required to be atomic to the
      * client. To.dir and from.dir must reside on the same file
      * system and server.
      *
      * If the directory, to_dir, already contains an entry with
      * the name, to_name, the source object must be compatible
      * with the target: either both are non-directories or both
      * are directories and the target must be empty. If
      * compatible, the existing target is removed before the
      * rename occurs. If they are not compatible or if the target
      * is a directory but not empty, the server should return the
      * error, NFS3ERR_EXIST.
      *
      * @param {Buffer} from_dir
      *                 The file handle for the directory from which the
      *                 entry is to be renamed.
      * @param {string} from_name
      *                 The name of the entry that identifies the object to
      *                 be renamed.
      * @param {Buffer} from_dir
      *                 The file handle for the directory to which the
      *                 object is to be renamed.
      * @param {string} from_name
      *                 The new name for the object.
      * @param {function} callback(err: null,
      *                            fromdir_wcc: Object,
      *                            todir_wcc: Object);
      * @param {function} callback(err: string,
      *                            fromdir_wcc: Object,
      *                            todir_wcc: Object);
      */
    rename(from_dir, from_name, to_dir, to_name, callback) {
        this.client.rename3(from_dir, from_name, to_dir, to_name, callback);
    }
}

module.exports = { V3 };
