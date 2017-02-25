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
 *    Vianney Rancurel <vr@scality.com>
 *    Guillaume Gimenez <ggim@scality.com>
 */
#include "node_nfsc_mount3.h"
#include "node_nfsc.h"
#include "node_nfsc_errors3.h"
#include <gssrpc/rpc.h>
#include "mount3.h"
#include "nfs3.h"

// ( callback(err, root_fh) )
NAN_METHOD(NFS::Client::Mount3) {
    bool typeError = true;
    if ( info.Length() != 1 ) {
        Nan::ThrowTypeError("Must be called with 1 parameters");
        return;
      }
    if (!info[0]->IsFunction()) {
        Nan::ThrowTypeError("Parameter 1, callback must be a function");
    }
    else
        typeError = false;
    if (typeError)
        return;
    NFS::Client* obj = ObjectWrap::Unwrap<NFS::Client>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[0].As<v8::Function>());
    Nan::AsyncQueueWorker(new NFS::Mount3Worker(obj, callback));
}

AUTH *NFS::Mount3Worker::createUnixAuth(int uid, int gid)
{
    char machname[MAX_MACHINE_NAME +1];
    int gids[1];

    if(gethostname(machname, MAX_MACHINE_NAME) == -1) {
        asprintf(&error, NFSC_EGETHOSTNAME);
        return NULL;
    }

    machname[MAX_MACHINE_NAME] = 0;
    gids[0] = gid;

    return authunix_create(machname, uid, gid, 1, (int *) gids);
}

CLIENT *NFS::Mount3Worker::createMountClient()
{
    struct sockaddr_in	server_addr, addr;
    int sock;
    CLIENT *mntclient;
    int ret;
    const char* host = client->getHost();
    const char* protocol = client->getProtocol();
    int uid = client->getUid();
    int gid = client->getGid();
    timeval& timeout = client->getTimeout();
    int udp = !strcmp(protocol, "udp");
    char hostBuf[2048];

    ret = inet_aton(host, &server_addr.sin_addr);
    if (0 == ret) {
        struct hostent hp, *result;
        int err;
        ret = gethostbyname_r(host, &hp, hostBuf, sizeof hostBuf, &result, &err);
        if (ret != 0 || result == NULL)
        {
            asprintf(&error, NFSC_EGETHOSTBYNAME": %s: %s", host, strerror(err));
            return(NULL);
        }
        memmove(&server_addr.sin_addr.s_addr, hp.h_addr, hp.h_length);
        host = hp.h_name;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = 0;

    if (udp)
    {
        sock = RPC_ANYSOCK;
        addr = server_addr;

        if ((mntclient = clntudp_create( &addr, MOUNT_PROGRAM, MOUNT_V3, timeout,
                                         &sock)) == (CLIENT *)0)
        {
            clnt_pcreateerror(const_cast<char*>("mnt_clntudp_create"));
            return(NULL);
        }
    }
    else
    {
        sock = RPC_ANYSOCK;
        addr = server_addr;

        if ((mntclient = clnttcp_create( &addr, MOUNT_PROGRAM, MOUNT_V3, &sock, 0,
                                         0)) == (CLIENT*)0)
        {
            clnt_pcreateerror(const_cast<char*>("mnt_clnttcp_create"));
            return NULL;
        }
    }

    clnt_control(mntclient, CLSET_TIMEOUT, (char *) &timeout);

    mntclient->cl_auth = createUnixAuth(uid, gid);
    if (mntclient->cl_auth == NULL)
    {
        clnt_destroy(mntclient);
        return(NULL);
    }

    return (mntclient);
}

CLIENT *NFS::Mount3Worker::createNfsClient()
{
    struct sockaddr_in	server_addr, addr;
    int			sock;
    CLIENT		*nfsclient;
    int ret;
    const char* host = client->getHost();
    const char* protocol = client->getProtocol();
    int uid = client->getUid();
    int gid = client->getGid();
    timeval& timeout = client->getTimeout();
    int udp = !strcmp(protocol, "udp");
    const char* authMethod = client->getAuthenticationMethod();
    char hostBuf[2048];

    ret = inet_aton(host, &server_addr.sin_addr);
    if (0 == ret) {
        struct hostent hp, *result;
        int err;
        ret = gethostbyname_r(host, &hp, hostBuf, sizeof hostBuf, &result, &err);
        if (ret != 0 || result == NULL)
        {
            asprintf(&error, NFSC_EGETHOSTBYNAME": %s: %s", host, strerror(err));
            return(NULL);
        }
        memmove(&server_addr.sin_addr.s_addr, hp.h_addr, hp.h_length);
        host = hp.h_name;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = 0;

    if (udp)
    {
        sock = RPC_ANYSOCK;
        addr = server_addr;
        if((nfsclient = clntudp_create( &addr, NFS_PROGRAM, NFS_V3, timeout,
                                        &sock)) == (CLIENT *)0)
        {
            clnt_pcreateerror(const_cast<char*>("nfs_clntudp_create"));
            return(NULL);
        }
    }
    else
    {
        sock = RPC_ANYSOCK;
        addr = server_addr;
        if((nfsclient = clnttcp_create( &addr, NFS_PROGRAM, NFS_V3, &sock, 0,
                                        0)) == (CLIENT*)0)
        {
            clnt_pcreateerror(const_cast<char*>("nfs_clnttcp_create"));
            return(NULL);
        }
    }

    clnt_control(nfsclient, CLSET_TIMEOUT, (char *) &timeout);

    if (0 == strcmp(authMethod, "none") ) {
        nfsclient->cl_auth = authnone_create();
    } else if (0 == strcmp(authMethod, "unix")) {

        nfsclient->cl_auth = createUnixAuth(uid, gid);
        if(nfsclient->cl_auth == NULL)
        {
            clnt_destroy(nfsclient);
            return(NULL);
        }
    } else {
        struct rpc_gss_sec sec;
        /* from kerberos source, gssapi_krb5.c */
        static gss_OID_desc krb5oid = {9, const_cast<char*>("\052\206\110\206\367\022\001\002\002")};
        sec.mech = (gss_OID) &krb5oid;
        sec.qop = GSS_C_QOP_DEFAULT;
        if (0 == strcmp(authMethod, "krb5")) {
            sec.svc = RPCSEC_GSS_SVC_NONE;
        } else if (0 == strcmp(authMethod, "krb5i")) {
            sec.svc = RPCSEC_GSS_SVC_INTEGRITY;
        } else if (0 == strcmp(authMethod, "krb5p")) {
            sec.svc = RPCSEC_GSS_SVC_PRIVACY;
        } else {
            asprintf(&error, "Invalid authentication method");
            clnt_destroy(nfsclient);
            return(NULL);
        }
        sec.cred = GSS_C_NO_CREDENTIAL;
        sec.req_flags = GSS_C_MUTUAL_FLAG;

        nfsclient->cl_auth = authgss_create_default(nfsclient,
                                                    const_cast<char*>("nfs"),
                                                    &sec);
        if (nfsclient->cl_auth == NULL)
        {
            clnt_pcreateerror(const_cast<char*>("authgss_create_default"));
            clnt_destroy(nfsclient);
            return NULL;
        }
    }
    return(nfsclient);
}

bool NFS::Mount3Worker::mount()
{
    const char *host = client->getHost();
    const char *dir = client->getExportPath();
    CLIENT *mntclient = NULL;
    CLIENT *nfsclient = NULL;
    GETATTR3res attr = {};
    GETATTR3args args = {};
    enum clnt_stat state;
    mountres3 mount_point;
    bool isMounted = false;
    memset(&mount_point, 0, sizeof mount_point);

    mntclient = createMountClient();
    if (NULL == mntclient)
      {
        goto bad;
      }

    state = mountproc3_mnt_3(const_cast<char**>(&dir), &mount_point, mntclient);
    if (state != RPC_SUCCESS)
      {
        asprintf(&error, "%s", rpc_error(state));
        goto bad;
      }

    if (MNT3_OK != mount_point.fhs_status)
      {
        asprintf(&error, "%s", mnt3_error(mount_point.fhs_status));
        goto bad;
      }
    isMounted = true;
    client->setRootFh(mount_point.mountres3_u.mountinfo.fhandle.fhandle3_val,
                      mount_point.mountres3_u.mountinfo.fhandle.fhandle3_len);

    nfsclient = createNfsClient();
    if (NULL == nfsclient)
     {
       goto bad;
     }
    {
    args.object = client->getRootFh();
    state = nfsproc3_getattr_3(&args, &attr, nfsclient);
 }
    if (state != RPC_SUCCESS)
      {
        asprintf(&error, "%s", rpc_error(state));
        goto bad;
      }

    if (NFS3_OK != attr.status)
      {
        asprintf(&error, "%s", nfs3_error(attr.status));
        goto bad;
      }
    client->setClient(nfsclient);
    auth_destroy(mntclient->cl_auth);
    clnt_destroy(mntclient);

    client->setMounted();
    return true;

   bad:
    if (isMounted) {
        char clnt_res;
        mountproc3_umnt_3(const_cast<char**>(&dir), &clnt_res, mntclient);
    }
    if (NULL != nfsclient)
      {
        if (NULL != nfsclient->cl_auth)
          auth_destroy(nfsclient->cl_auth);
        clnt_destroy(nfsclient);
      }

    if (NULL != mntclient)
      {
        if (NULL != mntclient->cl_auth)
          auth_destroy(mntclient->cl_auth);
        clnt_destroy(mntclient);
      }

    return false;
}

NFS::Mount3Worker::Mount3Worker(NFS::Client *client_,
                              Nan::Callback *callback)
    : Nan::AsyncWorker(callback),
      client(client_),
      success(false),
      error(0)
{

}

NFS::Mount3Worker::~Mount3Worker()
{
    free(error);
}

void NFS::Mount3Worker::Execute()
{
    if (client->isMounted()) {
        asprintf(&error, NFSC_ALREADY_MOUNTED);
        return;
    }
    Serialize my(client);
    success = mount();
}

void NFS::Mount3Worker::HandleOKCallback()
{
    Nan::HandleScope scope;

    if (success) {
        const nfs_fh3& root_fh = client->getRootFh();
        char *root_fh_buf = (char*)malloc(root_fh.data.data_len);
        memcpy(root_fh_buf, root_fh.data.data_val, root_fh.data.data_len);
        v8::Local<v8::Value> argv[] = {
            Nan::Null(),
            Nan::NewBuffer(root_fh_buf,
                           root_fh.data.data_len).ToLocalChecked()
        };
        callback->Call(sizeof(argv)/sizeof(*argv), argv);
    }
    else {
        v8::Local<v8::Value> argv[] = {
            Nan::New(error?error:NFSC_UNKNOWN_ERROR).ToLocalChecked()
        };
        callback->Call(1, argv);
    }
}
