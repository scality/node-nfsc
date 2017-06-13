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
#include "node_nfsc_mknod3.h"
#include "node_nfsc_sattr3.h"
#include "node_nfsc_fattr3.h"
#include "node_nfsc_wcc3.h"

// (dir, name, type, mknodData, callback(err, obj_fh, obj_attr, dir_wcc) )
NAN_METHOD(NFS::Client::MkNod3) {
    bool typeError = true;
    if ( info.Length() != 5) {
        Nan::ThrowTypeError("Must be called with 5 parameters");
        return;
    }
    if (!info[0]->IsUint8Array())
        Nan::ThrowTypeError("Parameter 1, dir must be a Buffer");
    else if (!info[1]->IsString())
        Nan::ThrowTypeError("Parameter 2, name must be a string");
    else if (!info[2]->IsString())
        Nan::ThrowTypeError("Parameter 3, mode must be an string");
    else if (!info[3]->IsObject())
        Nan::ThrowTypeError("Parameter 4, mknodData must be an object");
    else if (!info[4]->IsFunction())
        Nan::ThrowTypeError("Parameter 5, callback must be a function");
    else
        typeError = false;
    if (typeError)
        return;
    NFS::Client* obj = ObjectWrap::Unwrap<NFS::Client>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[4].As<v8::Function>());
    Nan::AsyncQueueWorker(new NFS::MkNod3Worker(obj, info[0], info[1],
                                                info[2], info[3], callback));
}

NFS::MkNod3Worker::MkNod3Worker(NFS::Client *client_,
                                const v8::Local<v8::Value> &parent_fh_,
                                const v8::Local<v8::Value> &name_,
                                const v8::Local<v8::Value> &type_,
                                const v8::Local<v8::Value> &mknodData_,
                                Nan::Callback *callback)
    : Procedure3Worker(client_, (xdrproc_t) xdr_MKNOD3res, callback),
      name(name_),
      type(type_)
{
    v8::Local<v8::Value> rdev;
    v8::Local<v8::Value> attrs;
    bool is_dev = false;
    bool known_type;
    args.where.dir.data.data_val = node::Buffer::Data(parent_fh_);
    args.where.dir.data.data_len = node::Buffer::Length(parent_fh_);
    args.where.name = *name;
    known_type = ftype3_value(*type, &args.what.type);
    if (!known_type) {
        Nan::ThrowTypeError("Unknown type");
        return;
    }
    switch (args.what.type) {
    default:
    case NF3REG:
    case NF3DIR:
    case NF3LNK:
        Nan::ThrowTypeError("Invalid creation mode");
        return;
    case NF3BLK:
    case NF3CHR:
        /* object is composed of {rdev, attrs} */
        rdev = v8::Local<v8::Object>::Cast(mknodData_)
                ->Get(Nan::New("rdev").ToLocalChecked());
        if (!rdev->IsObject()) {
            Nan::ThrowTypeError("Invalid argument in mknodData, "
                                "rdev is not an object");
            return;
        }
        is_dev = true;
        //fallthrough
    case NF3FIFO:
    case NF3SOCK:
        /* object is attrs */
        attrs = v8::Local<v8::Object>::Cast(mknodData_)
                ->Get(Nan::New("attrs").ToLocalChecked());
        if (!attrs->IsObject()) {
            Nan::ThrowTypeError("Invalid argument in mknodData, "
                                "attrs is not an object");
            return;
        }
    }

    if (is_dev) {
        args.what.mknoddata3_u.device.spec.specdata1 =
                v8::Local<v8::Object>::Cast(rdev)
                ->Get(Nan::New("major").ToLocalChecked())->Int32Value();
        args.what.mknoddata3_u.device.spec.specdata2 =
                v8::Local<v8::Object>::Cast(rdev)
                ->Get(Nan::New("minor").ToLocalChecked())->Int32Value();
        args.what.mknoddata3_u.device.dev_attributes =
                node_nfsc_sattr3(v8::Local<v8::Object>::Cast(attrs));
    } else {
        args.what.mknoddata3_u.pipe_attributes =
                node_nfsc_sattr3(v8::Local<v8::Object>::Cast(attrs));
    }
}

void NFS::MkNod3Worker::procSuccess()
{
    v8::Local<v8::Value> obj_fh;
    if (res.MKNOD3res_u.resok.obj.handle_follows) {
        obj_fh = Nan::NewBuffer(res.MKNOD3res_u.resok.obj.post_op_fh3_u
                                .handle.data.data_val,
                                res.MKNOD3res_u.resok.obj.post_op_fh3_u
                                .handle.data.data_len)
                .ToLocalChecked();
    } else {
        obj_fh = Nan::Null();
    }
    v8::Local<v8::Object> wcc = Nan::New<v8::Object>();
    v8::Local<v8::Value> before, after;
    if (res.MKNOD3res_u.resok.dir_wcc.before.attributes_follow)
        before = node_nfsc_wcc3(res.MKNOD3res_u.resok.dir_wcc.before
                                .pre_op_attr_u.attributes);
    else
        before = Nan::Null();
    if (res.MKNOD3res_u.resok.dir_wcc.after.attributes_follow)
        after = node_nfsc_fattr3(res.MKNOD3res_u.resok.dir_wcc.after
                                 .post_op_attr_u.attributes);
    else
        after = Nan::Null();
    wcc->Set(Nan::New("before").ToLocalChecked(), before);
    wcc->Set(Nan::New("after").ToLocalChecked(), after);

    v8::Local<v8::Value> obj_attrs;
    if (res.MKNOD3res_u.resok.obj_attributes.attributes_follow)
        obj_attrs = node_nfsc_fattr3(res.MKNOD3res_u.resok.obj_attributes
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
    res.MKNOD3res_u.resok.obj.post_op_fh3_u.handle.data.data_val = NULL;
    callback->Call(sizeof(argv)/sizeof(*argv), argv);
}

void NFS::MkNod3Worker::procFailure()
{
    v8::Local<v8::Object> wcc = Nan::New<v8::Object>();
    v8::Local<v8::Value> before, after;
    if (res.MKNOD3res_u.resfail.dir_wcc.before.attributes_follow)
        before = node_nfsc_wcc3(res.MKNOD3res_u.resfail.dir_wcc.before
                                .pre_op_attr_u.attributes);
    else
        before = Nan::Null();
    if (res.MKNOD3res_u.resfail.dir_wcc.after.attributes_follow)
        after = node_nfsc_fattr3(res.MKNOD3res_u.resfail.dir_wcc.after
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
