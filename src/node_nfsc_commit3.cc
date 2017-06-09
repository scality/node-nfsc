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
#include "node_nfsc_commit3.h"
#include "node_nfsc_fattr3.h"
#include "node_nfsc_wcc3.h"

// (object, count, offset, callback(null, verf, attrs))
// (object, count, offset, callback(err, attrs))
NAN_METHOD(NFS::Client::Commit3) {
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
    Nan::AsyncQueueWorker(new NFS::Commit3Worker(obj, info[0], info[1], info[2],
                                               callback));
}

NFS::Commit3Worker::Commit3Worker(NFS::Client *client_,
                                  const v8::Local<v8::Value> &obj_fh_,
                                  const v8::Local<v8::Value> &count_,
                                  const v8::Local<v8::Value> &offset_,
                                  Nan::Callback *callback)
    : Procedure3Worker(client_, (xdrproc_t) xdr_COMMIT3res, callback)
{
    args.file.data.data_val = node::Buffer::Data(obj_fh_);
    args.file.data.data_len = node::Buffer::Length(obj_fh_);
    args.count = count_->NumberValue();
    args.offset = CheckUDouble(offset_->NumberValue());
    if (args.offset == (uint64_t)-1) {
        Nan::ThrowRangeError("Invalid offset");
        return;
    }
}

void NFS::Commit3Worker::procSuccess()
{
    char * verf = (char*)malloc(NFS3_WRITEVERFSIZE);
    memcpy(verf, &res.COMMIT3res_u.resok.verf[0], NFS3_WRITEVERFSIZE);
    v8::Local<v8::Object> obj_attrs = Nan::New<v8::Object>();
    v8::Local<v8::Value> before, after;
    if (res.COMMIT3res_u.resok.file_wcc.before.attributes_follow)
        before = node_nfsc_wcc3(res.COMMIT3res_u.resok.file_wcc
                                .before.pre_op_attr_u.attributes);
    else
        before = Nan::Null();
    if (res.COMMIT3res_u.resok.file_wcc.after.attributes_follow)
        after = node_nfsc_fattr3(res.COMMIT3res_u.resok.file_wcc
                                 .after.post_op_attr_u.attributes);
    else
        after = Nan::Null();
    obj_attrs->Set(Nan::New("before").ToLocalChecked(),
                   before);
    obj_attrs->Set(Nan::New("after").ToLocalChecked(),
                   before);
    v8::Local<v8::Value> argv[] = {
        Nan::Null(),
        Nan::NewBuffer(verf, NFS3_WRITEVERFSIZE).ToLocalChecked(),
        obj_attrs
    };
    //data stolen by node
    callback->Call(sizeof(argv)/sizeof(*argv), argv);
}

void NFS::Commit3Worker::procFailure()
{
    v8::Local<v8::Object> obj_attrs = Nan::New<v8::Object>();
    v8::Local<v8::Value> before, after;
    if (res.COMMIT3res_u.resfail.file_wcc.before.attributes_follow)
        before = node_nfsc_wcc3(res.COMMIT3res_u.resfail
                                .file_wcc.before.pre_op_attr_u.attributes);
    else
        before = Nan::Null();
    if (res.COMMIT3res_u.resfail.file_wcc.after.attributes_follow)
        after = node_nfsc_fattr3(res.COMMIT3res_u.resfail
                                 .file_wcc.after.post_op_attr_u.attributes);
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

