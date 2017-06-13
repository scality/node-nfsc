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
#include "node_nfsc_mkdir3.h"
#include "node_nfsc_sattr3.h"
#include "node_nfsc_fattr3.h"
#include "node_nfsc_wcc3.h"

// (dir, name, attrs, callback(err, obj_fh, obj_attr, dir_wcc) )
NAN_METHOD(NFS::Client::MkDir3) {
    bool typeError = true;
    if ( info.Length() != 4) {
        Nan::ThrowTypeError("Must be called with 4 parameters");
        return;
    }
    if (!info[0]->IsUint8Array())
        Nan::ThrowTypeError("Parameter 1, dir must be a Buffer");
    else if (!info[1]->IsString())
        Nan::ThrowTypeError("Parameter 2, name must be a string");
    else if (!info[2]->IsObject())
        Nan::ThrowTypeError("Parameter 3, attrs must be an Object");
    else if (!info[3]->IsFunction())
        Nan::ThrowTypeError("Parameter 4, callback must be a function");
    else
        typeError = false;
    if (typeError)
        return;
    NFS::Client* obj = ObjectWrap::Unwrap<NFS::Client>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[3].As<v8::Function>());
    Nan::AsyncQueueWorker(new NFS::MkDir3Worker(obj, info[0], info[1],
                                                 info[2], callback));
}

NFS::MkDir3Worker::MkDir3Worker(NFS::Client *client_,
                                  const v8::Local<v8::Value> &parent_fh_,
                                  const v8::Local<v8::Value> &name_,
                                  const v8::Local<v8::Value> &attrs_,
                                  Nan::Callback *callback)
    : Procedure3Worker(client_, (xdrproc_t) xdr_MKDIR3res, callback),
      name(name_)
{
    args.where.dir.data.data_val = node::Buffer::Data(parent_fh_);
    args.where.dir.data.data_len = node::Buffer::Length(parent_fh_);
    args.where.name = *name;
    args.attributes = node_nfsc_sattr3(v8::Local<v8::Object>::Cast(attrs_));
}

void NFS::MkDir3Worker::procSuccess()
{
    v8::Local<v8::Value> obj_fh;
    if (res.MKDIR3res_u.resok.obj.handle_follows) {
        obj_fh = Nan::NewBuffer(res.MKDIR3res_u.resok.obj.post_op_fh3_u
                                .handle.data.data_val,
                                res.MKDIR3res_u.resok.obj.post_op_fh3_u
                                .handle.data.data_len)
                .ToLocalChecked();
    } else {
        obj_fh = Nan::Null();
    }
    v8::Local<v8::Object> wcc = Nan::New<v8::Object>();
    v8::Local<v8::Value> before, after;
    if (res.MKDIR3res_u.resok.dir_wcc.before.attributes_follow)
        before = node_nfsc_wcc3(res.MKDIR3res_u.resok.dir_wcc.before
                                .pre_op_attr_u.attributes);
    else
        before = Nan::Null();
    if (res.MKDIR3res_u.resok.dir_wcc.after.attributes_follow)
        after = node_nfsc_fattr3(res.MKDIR3res_u.resok.dir_wcc.after
                                 .post_op_attr_u.attributes);
    else
        after = Nan::Null();
    wcc->Set(Nan::New("before").ToLocalChecked(), before);
    wcc->Set(Nan::New("after").ToLocalChecked(), after);

    v8::Local<v8::Value> obj_attrs;
    if (res.MKDIR3res_u.resok.obj_attributes.attributes_follow)
        obj_attrs = node_nfsc_fattr3(res.MKDIR3res_u.resok.obj_attributes
                                     .post_op_attr_u.attributes);
    else
        obj_attrs = Nan::Null();

    v8::Local<v8::Value> argv[] = {
        Nan::Null(),
        obj_fh,
        obj_attrs,
        wcc,
    };
    //data stolen by node
    res.MKDIR3res_u.resok.obj.post_op_fh3_u.handle.data.data_val = NULL;
    callback->Call(sizeof(argv)/sizeof(*argv), argv);
}

void NFS::MkDir3Worker::procFailure()
{
    v8::Local<v8::Object> wcc = Nan::New<v8::Object>();
    v8::Local<v8::Value> before, after;
    if (res.MKDIR3res_u.resfail.dir_wcc.before.attributes_follow)
        before = node_nfsc_wcc3(res.MKDIR3res_u.resfail.dir_wcc.before
                                .pre_op_attr_u.attributes);
    else
        before = Nan::Null();
    if (res.MKDIR3res_u.resfail.dir_wcc.after.attributes_follow)
        after = node_nfsc_fattr3(res.MKDIR3res_u.resfail.dir_wcc.after
                                 .post_op_attr_u.attributes);
    else
        after = Nan::Null();
    wcc->Set(Nan::New("before").ToLocalChecked(), before);
    wcc->Set(Nan::New("after").ToLocalChecked(), after);
    v8::Local<v8::Value> argv[] = {
        Nan::New(error?error:NFSC_UNKNOWN_ERROR).ToLocalChecked(),
        wcc
    };
    callback->Call(2, argv);
}

