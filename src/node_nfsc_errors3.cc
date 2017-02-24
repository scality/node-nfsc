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
#include "node_nfsc_errors3.h"
#include "nfs3.h"

const char *
rpc_error(clnt_stat stat)
{
    switch (stat)
    {
    case RPC_SUCCESS:
        return "RPC_SUCCESS";

        /*
         * local errors
         */
    case RPC_CANTENCODEARGS:
        return "RPC_CANTENCODEARGS";
    case RPC_CANTDECODERES:
        return "RPC_CANTDECODERES";
    case RPC_CANTSEND:
        return "RPC_CANTSEND";
    case RPC_CANTRECV:
        return "RPC_CANTRECV";
    case RPC_TIMEDOUT:
        return "RPC_TIMEDOUT";

        /*
         * remote errors
         */
    case RPC_VERSMISMATCH:
        return "RPC_VERSMISMATCH";
    case RPC_AUTHERROR:
        return "RPC_AUTHERROR";
    case RPC_PROGUNAVAIL:
        return "RPC_PROGUNAVAIL";
    case RPC_PROGVERSMISMATCH:
        return "RPC_PROGVERSMISMATCH";
    case RPC_PROCUNAVAIL:
        return "RPC_PROCUNAVAIL";
    case RPC_CANTDECODEARGS:
        return "RPC_CANTDECODEARGS";
    case RPC_SYSTEMERROR:
        return "RPC_SYSTEMERROR";

        /*
         * callrpc & clnt_create errors
         */
    case RPC_UNKNOWNHOST:
        return "RPC_UNKNOWNHOST";
    case RPC_UNKNOWNPROTO:
        return "RPC_UNKNOWNPROTO";

        /*
         * rpcbind errors
         */
    case RPC_PROGNOTREGISTERED:
        return "RPC_PROGNOTREGISTERED";
    case RPC_PMAPFAILURE:
        return "RPC_PMAPFAILURE";
        /*
         * unspecified error
         */
    case RPC_FAILED:
        return "RPC_FAILED";
    default:
        return "RPC unknown error";
    }
}

const char *
mnt3_error(mountstat3 stat)
{
    switch (stat)
    {
    case MNT3_OK:
        return "MNT3_OK";
    case MNT3ERR_PERM:
        return "MNT3ERR_PERM";
    case MNT3ERR_NOENT:
        return "MNT3ERR_NOENT";
    case MNT3ERR_IO:
        return "MNT3ERR_IO";
    case MNT3ERR_ACCES:
        return "MNT3ERR_ACCES";
    case MNT3ERR_NOTDIR:
        return "MNT3ERR_NOTDIR";
    case MNT3ERR_INVAL:
        return "MNT3ERR_INVAL";
    case MNT3ERR_NAMETOOLONG:
        return "MNT3ERR_NAMETOOLONG";
    case MNT3ERR_NOTSUPP:
        return "MNT3ERR_NOTSUPP";
    case MNT3ERR_SERVERFAULT:
        return "MNT3ERR_SERVERFAULT";
    default:
        return "MNT3 unknown error";
    }
}

const char *
nfs3_error(nfsstat3 stat)
{
    switch (stat)
    {
    case NFS3_OK:
        return "NFS3_OK";
    case NFS3ERR_PERM:
        return "NFS3ERR_PERM";
    case NFS3ERR_NOENT:
        return "NFS3ERR_NOENT";
    case NFS3ERR_IO:
        return "NFS3ERR_IO";
    case NFS3ERR_NXIO:
        return "NFS3ERR_NXIO";
    case NFS3ERR_ACCES:
        return "NFS3ERR_ACCES";
    case NFS3ERR_EXIST:
        return "NFS3ERR_EXIST";
    case NFS3ERR_XDEV:
        return "NFS3ERR_XDEV";
    case NFS3ERR_NODEV:
        return "NFS3ERR_NODEV";
    case NFS3ERR_NOTDIR:
        return "NFS3ERR_NOTDIR";
    case NFS3ERR_ISDIR:
        return "NFS3ERR_ISDIR";
    case NFS3ERR_INVAL:
        return "NFS3ERR_INVAL";
    case NFS3ERR_FBIG:
        return "NFS3ERR_FBIG";
    case NFS3ERR_NOSPC:
        return "NFS3ERR_NOSPC";
    case NFS3ERR_ROFS:
        return "NFS3ERR_ROFS";
    case NFS3ERR_MLINK:
        return "NFS3ERR_MLINK";
    case NFS3ERR_NAMETOOLONG:
        return "NFS3ERR_NAMETOOLONG";
    case NFS3ERR_NOTEMPTY:
        return "NFS3ERR_NOTEMPTY";
    case NFS3ERR_DQUOT:
        return "NFS3ERR_DQUOT";
    case NFS3ERR_STALE:
        return "NFS3ERR_STALE";
    case NFS3ERR_REMOTE:
        return "NFS3ERR_REMOTE";
    case NFS3ERR_BADHANDLE:
        return "NFS3ERR_BADHANDLE";
    case NFS3ERR_NOT_SYNC:
        return "NFS3ERR_NOT_SYNC";
    case NFS3ERR_BAD_COOKIE:
        return "NFS3ERR_BAD_COOKIE";
    case NFS3ERR_NOTSUPP:
        return "NFS3ERR_NOTSUPP";
    case NFS3ERR_TOOSMALL:
        return "NFS3ERR_TOOSMALL";
    case NFS3ERR_SERVERFAULT:
        return "NFS3ERR_SERVERFAULT";
    case NFS3ERR_BADTYPE:
        return "NFS3ERR_BADTYPE";
    case NFS3ERR_JUKEBOX:
        return "NFS3ERR_JUKEBOX";
    default:
        return "NFS3 unknown error";
    }
}
