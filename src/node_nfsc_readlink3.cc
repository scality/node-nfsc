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
#include "node_nfsc_readlink3.h"
#include "node_nfsc_fattr3.h"

// (object, callback(err, obj_attr) )
NAN_METHOD(NFS::Client::ReadLink3) {
    bool typeError = true;
    if ( info.Length() != 2) {
        Nan::ThrowTypeError("Must be called with 2 parameters");
        return;
    }
    if (!info[0]->IsUint8Array())
        Nan::ThrowTypeError("Parameter 1, object must be a Buffer");
    else if (!info[1]->IsFunction())
        Nan::ThrowTypeError("Parameter 2, callback must be a function");
    else
        typeError = false;
    if (typeError)
        return;
    NFS::Client* obj = ObjectWrap::Unwrap<NFS::Client>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[1].As<v8::Function>());
    Nan::AsyncQueueWorker(new NFS::ReadLink3Worker(obj, info[0], callback));
}

NFS::ReadLink3Worker::ReadLink3Worker(NFS::Client *client_,
                                      const v8::Local<v8::Value> &obj_fh_,
                                      Nan::Callback *callback)
    : Procedure3Worker(client_, (xdrproc_t) xdr_READLINK3res, callback)
{
    args.symlink.data.data_val = node::Buffer::Data(obj_fh_);
    args.symlink.data.data_len = node::Buffer::Length(obj_fh_);
}

void NFS::ReadLink3Worker::procSuccess()
{
    v8::Local<v8::Value> data;
    data = Nan::New(res.READLINK3res_u.resok.data)
            .ToLocalChecked();

    v8::Local<v8::Value> obj_attrs;
    if (res.READLINK3res_u.resok.symlink_attributes.attributes_follow)
        obj_attrs = node_nfsc_fattr3(res.READLINK3res_u.resok
                                     .symlink_attributes.post_op_attr_u
                                     .attributes);
    else
        obj_attrs = Nan::Null();
    v8::Local<v8::Value> argv[] = {
        Nan::Null(),
        data,
        obj_attrs,
    };
    callback->Call(sizeof(argv)/sizeof(*argv), argv);
}

void NFS::ReadLink3Worker::procFailure()
{
    v8::Local<v8::Value> obj_attrs;
    if (res.READLINK3res_u.resfail.symlink_attributes.attributes_follow)
        obj_attrs = node_nfsc_fattr3(res.READLINK3res_u.resfail
                                     .symlink_attributes.post_op_attr_u
                                     .attributes);
    else
        obj_attrs = Nan::Null();
    v8::Local<v8::Value> argv[] = {
        Nan::New(error?error:NFSC_UNKNOWN_ERROR).ToLocalChecked(),
        obj_attrs
    };
    callback->Call(sizeof(argv)/sizeof(*argv), argv);
}
