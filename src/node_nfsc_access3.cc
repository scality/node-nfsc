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
#include "node_nfsc_access3.h"
#include "node_nfsc_errors3.h"
#include "node_nfsc_fattr3.h"

NFS::Access3Worker::Access3Worker(NFS::Client *client_,
                                const v8::Local<v8::Value> &obj_fh_,
                                const v8::Local<v8::Value> &access_,
                                Nan::Callback *callback)
    : Nan::AsyncWorker(callback),
      client(client_),
      success(false),
      error(0),
      obj_fh(),
      access(access_->Uint32Value()),
      res({})
{
    obj_fh.data.data_val = node::Buffer::Data(obj_fh_);
    obj_fh.data.data_len = node::Buffer::Length(obj_fh_);
}

NFS::Access3Worker::~Access3Worker()
{
    free(error);
    clnt_freeres(client->getClient(), (xdrproc_t) xdr_ACCESS3res, (char *) &res);
}

void NFS::Access3Worker::Execute()
{
    if (!client->isMounted()) {
        asprintf(&error, "Not mounted");
        return;
    }
    Serialize my(client);
    ACCESS3args args;
    clnt_stat stat;
    args.object = obj_fh;
    args.access = access;
    stat = nfsproc3_access_3(&args, &res, client->getClient());
    if (stat != RPC_SUCCESS) {
        asprintf(&error, "RPC: access failure: %s", rpc_error(stat));
        return;
    }
    if (res.status != NFS3_OK) {
        asprintf(&error, "NFS: access failure: %s", nfs3_error(res.status));
        return;
    }
    success = true;
}

void NFS::Access3Worker::HandleOKCallback()
{
    Nan::HandleScope scope;
    if (success) {
        v8::Local<v8::Object> obj_attrs =
                node_nfsc_fattr3(res.ACCESS3res_u.resok.obj_attributes.post_op_attr_u.attributes);
        v8::Local<v8::Value> argv[] = {
            Nan::Null(),
            Nan::New(res.ACCESS3res_u.resok.access),
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

