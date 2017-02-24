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
#include "node_nfsc_lookup3.h"
#include "node_nfsc_errors3.h"
#include "node_nfsc_fattr3.h"

NFS::Lookup3Worker::Lookup3Worker(NFS::Client *client_,
                                const v8::Local<v8::Value> &parent_fh_,
                                const v8::Local<v8::Value> &name_,
                                Nan::Callback *callback)
    : Nan::AsyncWorker(callback),
      client(client_),
      success(false),
      error(0),
      parent_fh(),
      name(name_),
      res({})
{
    parent_fh.data.data_val = node::Buffer::Data(parent_fh_);
    parent_fh.data.data_len = node::Buffer::Length(parent_fh_);
}

NFS::Lookup3Worker::~Lookup3Worker()
{
    free(error);
    clnt_freeres(client->getClient(), (xdrproc_t) xdr_LOOKUP3res, (char *) &res);
}

void NFS::Lookup3Worker::Execute()
{
    if (!client->isMounted()) {
        asprintf(&error, "Not mounted");
        return;
    }
    Serialize my(client);
    LOOKUP3args args;
    clnt_stat stat;
    args.what.dir = parent_fh;
    args.what.name = *name;
    stat = nfsproc3_lookup_3(&args, &res, client->getClient());
    if (stat != RPC_SUCCESS) {
        asprintf(&error, "RPC: lookup failure: %s", rpc_error(stat));
        return;
    }
    if (res.status != NFS3_OK) {
        asprintf(&error, "NFS: lookup failure: %s", nfs3_error(res.status));
        return;
    }
    success = true;
}

void NFS::Lookup3Worker::HandleOKCallback()
{
    Nan::HandleScope scope;
    if (success) {
        v8::Local<v8::Object> obj_attrs =
                node_nfsc_fattr3(res.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes);
        v8::Local<v8::Object> dir_attrs =
                node_nfsc_fattr3(res.LOOKUP3res_u.resok.dir_attributes.post_op_attr_u.attributes);
        v8::Local<v8::Value> argv[] = {
            Nan::Null(),
            Nan::NewBuffer(res.LOOKUP3res_u.resok.object.data.data_val,
                           res.LOOKUP3res_u.resok.object.data.data_len).ToLocalChecked(), //obj_fh
            obj_attrs,
            dir_attrs
        };
        //data stolen by node
        res.LOOKUP3res_u.resok.object.data.data_val = nullptr;
        callback->Call(sizeof(argv)/sizeof(*argv), argv);
    }
    else {
        v8::Local<v8::Value> argv[] = {
            Nan::New(error?error:"Unspecified error").ToLocalChecked()
        };
        callback->Call(1, argv);
    }
}

