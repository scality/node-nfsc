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
#include "node_nfsc.h"
#include "node_nfsc_getattr3.h"
#include "node_nfsc_errors3.h"
#include "node_nfsc_fattr3.h"

NFS::GetAttr3Worker::GetAttr3Worker(NFS::Client *client_,
                                const v8::Local<v8::Value> &obj_fh_,
                                Nan::Callback *callback)
    : Nan::AsyncWorker(callback),
      client(client_),
      success(false),
      error(0),
      obj_fh(),
      res({})
{
    obj_fh.data.data_val = node::Buffer::Data(obj_fh_);
    obj_fh.data.data_len = node::Buffer::Length(obj_fh_);
}

NFS::GetAttr3Worker::~GetAttr3Worker()
{
    free(error);
    clnt_freeres(client->getClient(), (xdrproc_t) xdr_GETATTR3res, (char *) &res);
}

void NFS::GetAttr3Worker::Execute()
{
    if (!client->isMounted()) {
        asprintf(&error, "Not mounted");
        return;
    }
    Serialize my(client);
    GETATTR3args args;
    clnt_stat stat;
    args.object = obj_fh;
    stat = nfsproc3_getattr_3(&args, &res, client->getClient());
    if (stat != RPC_SUCCESS) {
        asprintf(&error, "RPC: getattr failure: %s", rpc_error(stat));
        return;
    }
    if (res.status != NFS3_OK) {
        asprintf(&error, "NFS: getattr failure: %s", nfs3_error(res.status));
        return;
    }
    success = true;
}

void NFS::GetAttr3Worker::HandleOKCallback()
{
    Nan::HandleScope scope;
    if (success) {
        v8::Local<v8::Object> obj_attrs =
                node_nfsc_fattr3(res.GETATTR3res_u.resok.obj_attributes);
        v8::Local<v8::Value> argv[] = {
            Nan::Null(),
            obj_attrs,
        };
        callback->Call(sizeof(argv)/sizeof(*argv), argv);
    }
    else {
        v8::Local<v8::Value> argv[] = {
            Nan::New(error?error:"Unspecified error").ToLocalChecked()
        };
        callback->Call(1, argv);
    }
}

