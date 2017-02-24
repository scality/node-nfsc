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
#include "node_nfsc_read3.h"
#include "node_nfsc_errors3.h"
#include "node_nfsc_fattr3.h"

NFS::Read3Worker::Read3Worker(NFS::Client *client_,
                            const v8::Local<v8::Value> &obj_fh_,
                            const v8::Local<v8::Value> &count_,
                            const v8::Local<v8::Value> &offset_,
                            Nan::Callback *callback)
    : Nan::AsyncWorker(callback),
      client(client_),
      success(false),
      error(0),
      obj_fh(),
      count(CheckUDouble(count_->NumberValue())),
      offset(CheckUDouble(offset_->NumberValue())),
      res({})
{
    obj_fh.data.data_val = node::Buffer::Data(obj_fh_);
    obj_fh.data.data_len = node::Buffer::Length(obj_fh_);
}

NFS::Read3Worker::~Read3Worker()
{
    free(error);
    clnt_freeres(client->getClient(), (xdrproc_t) xdr_GETATTR3res, (char *) &res);
}

void NFS::Read3Worker::Execute()
{
    if (!client->isMounted()) {
        asprintf(&error, "Not mounted");
        return;
    }
    if (count == (uint64_t)-1) {
        asprintf(&error, "count: ERANGE");
        return;
    }
    if (offset == (uint64_t)-1) {
        asprintf(&error, "offset: ERANGE");
        return;
    }
    Serialize my(client);
    READ3args args;
    args.file = obj_fh;
    args.count = count;
    args.offset = offset;
    clnt_stat stat;
    stat = nfsproc3_read_3(&args, &res, client->getClient());
    if (stat != RPC_SUCCESS) {
        asprintf(&error, "RPC: read failure: %s", rpc_error(stat));
        return;
    }
    if (res.status != NFS3_OK) {
        asprintf(&error, "NFS: read failure: %s", nfs3_error(res.status));
        return;
    }
    success = true;
}

void NFS::Read3Worker::HandleOKCallback()
{
    Nan::HandleScope scope;
    if (success) {
        v8::Local<v8::Object> obj_attrs =
                node_nfsc_fattr3(res.READ3res_u.resok.file_attributes.post_op_attr_u.attributes);
        v8::Local<v8::Value> argv[] = {
            Nan::Null(),
            Nan::New(!!res.READ3res_u.resok.eof),
            Nan::NewBuffer(res.READ3res_u.resok.data.data_val,
                           res.READ3res_u.resok.data.data_len).ToLocalChecked(),
            obj_attrs
        };
        //data stolen by node
        res.READ3res_u.resok.data.data_val = nullptr;
        callback->Call(sizeof(argv)/sizeof(*argv), argv);
    }
    else {
        v8::Local<v8::Value> argv[] = {
            Nan::New(error?error:"Unspecified error").ToLocalChecked()
        };
        callback->Call(1, argv);
    }
}

