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
 *    Jeremy Desanlis <jeremy.desanlis@scality.com>
 */
#include "node_nfsc.h"
#include "node_nfsc_fsstat3.h"
#include "node_nfsc_fattr3.h"

// (fsroot, callback(err, obj_attributes,
//                   tbytes, fbytes, abytes, tfiles, ffiles, afiles,
//                   invarsec) )
NAN_METHOD(NFS::Client::FsStat3) {
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
    Nan::AsyncQueueWorker(new NFS::FsStat3Worker(obj, info[0], callback));
}

NFS::FsStat3Worker::FsStat3Worker(NFS::Client *client_,
                                const v8::Local<v8::Value> &fsroot_,
                                Nan::Callback *callback)
    : Procedure3Worker(client_, (xdrproc_t) xdr_FSSTAT3res, callback)
{
    args.fsroot.data.data_val = node::Buffer::Data(fsroot_);
    args.fsroot.data.data_len = node::Buffer::Length(fsroot_);
}

void NFS::FsStat3Worker::procSuccess()
{
    v8::Local<v8::Value> obj_attrs;
    if (res.FSSTAT3res_u.resfail.obj_attributes.attributes_follow)
        obj_attrs = node_nfsc_fattr3(res.FSSTAT3res_u.resfail.obj_attributes
                                     .post_op_attr_u.attributes);
    else
        obj_attrs = Nan::Null();

    v8::Local<v8::Value> argv[] = {
        Nan::Null(),
        obj_attrs,
        Nan::New(double(res.FSSTAT3res_u.resok.tbytes)),
        Nan::New(double(res.FSSTAT3res_u.resok.fbytes)),
        Nan::New(double(res.FSSTAT3res_u.resok.abytes)),
        Nan::New(double(res.FSSTAT3res_u.resok.tfiles)),
        Nan::New(double(res.FSSTAT3res_u.resok.ffiles)),
        Nan::New(double(res.FSSTAT3res_u.resok.afiles)),
        Nan::New(double(res.FSSTAT3res_u.resok.invarsec))
    };
    callback->Call(sizeof(argv)/sizeof(*argv), argv);
}

void NFS::FsStat3Worker::procFailure()
{
    v8::Local<v8::Value> obj_attrs;
    if (res.FSSTAT3res_u.resok.obj_attributes.attributes_follow)
        obj_attrs = node_nfsc_fattr3(res.FSSTAT3res_u.resok.obj_attributes
                                     .post_op_attr_u.attributes);
    else
        obj_attrs = Nan::Null();

    v8::Local<v8::Value> argv[] = {
        Nan::New(error?error:NFSC_UNKNOWN_ERROR).ToLocalChecked(),
        obj_attrs
    };
    callback->Call(2, argv);
}
