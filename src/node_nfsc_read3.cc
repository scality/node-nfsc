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

// (object, count, offset, callback(err, eof, buf, obj_attr) )
NAN_METHOD(NFS::Client::Read3) {
    bool typeError = true;
    if ( info.Length() != 4) {
        Nan::ThrowTypeError("Must be called with 4 parameters");
        return;
    }
    if (!info[0]->IsUint8Array())
        Nan::ThrowTypeError("Parameter 1, object must be a Buffer");
    else if (!info[1]->IsNumber())
        Nan::ThrowTypeError("Parameter 2, count must be a unsigned integer");
    else if (!info[2]->IsNumber())
        Nan::ThrowTypeError("Parameter 3, offset must be a unsigned integer");
    else if (!info[3]->IsFunction())
        Nan::ThrowTypeError("Parameter 4, callback must be a function");
    else
        typeError = false;
    if (typeError)
        return;
    NFS::Client* obj = ObjectWrap::Unwrap<NFS::Client>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[3].As<v8::Function>());
    Nan::AsyncQueueWorker(new NFS::Read3Worker(obj, info[0], info[1], info[2], callback));
}

NFS::Read3Worker::Read3Worker(NFS::Client *client_,
                            const v8::Local<v8::Value> &obj_fh_,
                            const v8::Local<v8::Value> &count_,
                            const v8::Local<v8::Value> &offset_,
                            Nan::Callback *callback)
    : Nan::AsyncWorker(callback),
      client(client_),
      success(false),
      error(0),
      res({}),
      args({})
{
    args.file.data.data_val = node::Buffer::Data(obj_fh_);
    args.file.data.data_len = node::Buffer::Length(obj_fh_);
    args.count = count_->NumberValue();
    args.offset = CheckUDouble(offset_->NumberValue());
    if (args.offset == (uint64_t)-1) {
        Nan::ThrowRangeError("Invalid offset");
    }
}

NFS::Read3Worker::~Read3Worker()
{
    free(error);
    clnt_freeres(client->getClient(), (xdrproc_t) xdr_READ3res, (char *) &res);
}

void NFS::Read3Worker::Execute()
{
    if (!client->isMounted()) {
        NFSC_ASPRINTF(&error, NFSC_NOT_MOUNTED);
        return;
    }
    Serialize my(client);
    clnt_stat stat;
    stat = nfsproc3_read_3(&args, &res, client->getClient());
    if (stat != RPC_SUCCESS) {
        NFSC_ASPRINTF(&error, "%s", rpc_error(stat));
        return;
    }
    if (res.status != NFS3_OK) {
        NFSC_ASPRINTF(&error, "%s", nfs3_error(res.status));
        return;
    }
    success = true;
}

void NFS::Read3Worker::HandleOKCallback()
{
    Nan::HandleScope scope;
    if (success) {
        v8::Local<v8::Value> obj_attrs;
        if (res.READ3res_u.resok.file_attributes.attributes_follow)
            obj_attrs = node_nfsc_fattr3(res.READ3res_u.resok.file_attributes.post_op_attr_u.attributes);
        else
            obj_attrs = Nan::Null();
        v8::Local<v8::Value> argv[] = {
            Nan::Null(),
            Nan::New(!!res.READ3res_u.resok.eof),
            Nan::NewBuffer(res.READ3res_u.resok.data.data_val,
                           res.READ3res_u.resok.data.data_len).ToLocalChecked(),
            obj_attrs
        };
        //data stolen by node
        res.READ3res_u.resok.data.data_val = NULL;
        callback->Call(sizeof(argv)/sizeof(*argv), argv);
    }
    else {
        v8::Local<v8::Value> argv[] = {
            Nan::New(error?error:NFSC_UNKNOWN_ERROR).ToLocalChecked()
        };
        callback->Call(1, argv);
    }
}

