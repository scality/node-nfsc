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
#include "node_nfsc_write3.h"
#include "node_nfsc_errors3.h"
#include "node_nfsc_fattr3.h"
#include "node_nfsc_wcc3.h"

// (object, count, offset, stable, data, callback(null, commited, count, verf, attrs))
// (object, count, offset, stable, data, callback(err, attrs))
NAN_METHOD(NFS::Client::Write3) {
    bool typeError = true;
    if ( info.Length() != 6) {
        Nan::ThrowTypeError("Must be called with 6 parameters");
        return;
    }
    if (!info[0]->IsUint8Array())
        Nan::ThrowTypeError("Parameter 1, object must be a Buffer");
    else if (!info[1]->IsNumber())
        Nan::ThrowTypeError("Parameter 2, count must be a unsigned integer");
    else if (!info[2]->IsNumber())
        Nan::ThrowTypeError("Parameter 3, offset must be a unsigned integer");
    else if (!info[3]->IsInt32())
        Nan::ThrowTypeError("Parameter 4, stable must be a Integer");
    else if (!info[4]->IsUint8Array())
        Nan::ThrowTypeError("Parameter 5, data must be a Buffer");
    else if (!info[5]->IsFunction())
        Nan::ThrowTypeError("Parameter 6, callback must be a function");
    else
        typeError = false;
    if (typeError)
        return;
    NFS::Client* obj = ObjectWrap::Unwrap<NFS::Client>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[5].As<v8::Function>());
    Nan::AsyncQueueWorker(new NFS::Write3Worker(obj, info[0], info[1], info[2],
                                               info[3], info[4], callback));
}

NFS::Write3Worker::Write3Worker(NFS::Client *client_,
                              const v8::Local<v8::Value> &obj_fh_,
                              const v8::Local<v8::Value> &count_,
                              const v8::Local<v8::Value> &offset_,
                              const v8::Local<v8::Value> &stable_,
                              const v8::Local<v8::Value> &data_,
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
    args.stable = stable_how(stable_->Int32Value());
    args.data.data_val = const_cast<char*>(node::Buffer::Data(data_));
    args.data.data_len = node::Buffer::Length(data_);

    if (args.offset == (uint64_t)-1) {
        Nan::ThrowRangeError("Invalid offset");
        return;
    }
    if (node::Buffer::Length(data_) < args.count) {
        Nan::ThrowRangeError("count greater than buffer size");
        return;
    }
    switch (args.stable) {
    case UNSTABLE:
    case DATA_SYNC:
    case FILE_SYNC:
        break;
    default:
        Nan::ThrowRangeError("Invalid stable value");
        return;
    }
}

NFS::Write3Worker::~Write3Worker()
{
    free(error);
    clnt_freeres(client->getClient(), (xdrproc_t) xdr_WRITE3res, (char *) &res);
}

void NFS::Write3Worker::Execute()
{
    if (error)
        return;
    if (!client->isMounted()) {
        NFSC_ASPRINTF(&error, NFSC_NOT_MOUNTED);
        return;
    }

    Serialize my(client);
    clnt_stat stat;
    stat = nfsproc3_write_3(&args, &res, client->getClient());
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

void NFS::Write3Worker::HandleOKCallback()
{
    Nan::HandleScope scope;
    if (success) {
        char * verf = (char*)malloc(NFS3_WRITEVERFSIZE);
        memcpy(verf, &res.WRITE3res_u.resok.verf[0], NFS3_WRITEVERFSIZE);
        v8::Local<v8::Object> obj_attrs = Nan::New<v8::Object>();
        v8::Local<v8::Value> before, after;
        if (res.WRITE3res_u.resok.file_wcc.before.attributes_follow)
            before = node_nfsc_wcc3(res.WRITE3res_u.resok.file_wcc.before.pre_op_attr_u.attributes);
        else
            before = Nan::Null();
        if (res.WRITE3res_u.resok.file_wcc.after.attributes_follow)
            after = node_nfsc_fattr3(res.WRITE3res_u.resok.file_wcc.after.post_op_attr_u.attributes);
        else
            after = Nan::Null();
        obj_attrs->Set(Nan::New("before").ToLocalChecked(),
                       before);
        obj_attrs->Set(Nan::New("after").ToLocalChecked(),
                       before);
        v8::Local<v8::Value> argv[] = {
            Nan::Null(),
            Nan::New(res.WRITE3res_u.resok.committed),
            Nan::New(res.WRITE3res_u.resok.count),
            Nan::NewBuffer(verf, NFS3_WRITEVERFSIZE).ToLocalChecked(),
            obj_attrs
        };
        //data stolen by node
        callback->Call(sizeof(argv)/sizeof(*argv), argv);
    }
    else {
        v8::Local<v8::Object> obj_attrs = Nan::New<v8::Object>();
        v8::Local<v8::Value> before, after;
        if (res.WRITE3res_u.resfail.file_wcc.before.attributes_follow)
            before = node_nfsc_wcc3(res.WRITE3res_u.resfail.file_wcc.before.pre_op_attr_u.attributes);
        else
            before = Nan::Null();
        if (res.WRITE3res_u.resfail.file_wcc.after.attributes_follow)
            after = node_nfsc_fattr3(res.WRITE3res_u.resfail.file_wcc.after.post_op_attr_u.attributes);
        else
            after = Nan::Null();
        obj_attrs->Set(Nan::New("before").ToLocalChecked(),
                       before);
        obj_attrs->Set(Nan::New("after").ToLocalChecked(),
                       before);
        v8::Local<v8::Value> argv[] = {
            Nan::New(error?error:NFSC_UNKNOWN_ERROR).ToLocalChecked(),
            obj_attrs
        };
        callback->Call(2, argv);
    }
}

