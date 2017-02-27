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
#include "node_nfsc_create3.h"
#include "node_nfsc_errors3.h"
#include "node_nfsc_sattr3.h"
#include "node_nfsc_fattr3.h"
#include "node_nfsc_wcc3.h"

// (dir, name, mode, attrs|verifier, callback(err, obj_fh, obj_attr, dir_wcc) )
NAN_METHOD(NFS::Client::Create3) {
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
        Nan::ThrowTypeError("Parameter 3, mode must be an integer");
    /* info[3] check is done later in the constructor */
    else if (!info[4]->IsFunction())
        Nan::ThrowTypeError("Parameter 5, callback must be a function");
    else
        typeError = false;
    if (typeError)
        return;
    NFS::Client* obj = ObjectWrap::Unwrap<NFS::Client>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[4].As<v8::Function>());
    Nan::AsyncQueueWorker(new NFS::Create3Worker(obj, info[0], info[1],
                                                 info[2], info[3], callback));
}

NFS::Create3Worker::Create3Worker(NFS::Client *client_,
                                  const v8::Local<v8::Value> &parent_fh_,
                                  const v8::Local<v8::Value> &name_,
                                  const v8::Local<v8::Value> &mode_,
                                  const v8::Local<v8::Value> &attrs_,
                                  Nan::Callback *callback)
    : Nan::AsyncWorker(callback),
      client(client_),
      success(false),
      error(0),
      parent_fh(),
      name(name_),
      mode((createmode3)mode_->Int32Value()),
      attrs(attrs_),
      verf(nullptr),
      res({})
{
    parent_fh.data.data_val = node::Buffer::Data(parent_fh_);
    parent_fh.data.data_len = node::Buffer::Length(parent_fh_);
    switch (mode) {
    case UNCHECKED:
    case GUARDED:
        if (!attrs->IsUint8Array()) {
            Nan::ThrowTypeError("Invalid argument for this creation mode,"
                                " must be an Object when UNCHECKED/GUARDED");
            return;
        }
        break;
    case EXCLUSIVE:
        if (!attrs->IsUint8Array()) {
            Nan::ThrowTypeError("Invalid argument for this creation mode,"
                                " must be a Buffer when EXCLUSIVE");
            return;
        }
        if (node::Buffer::Length(attrs_) != NFS3_CREATEVERFSIZE) {
            Nan::ThrowTypeError("Invalid verifier size, must be 8 bytes long");
            return;
        }
        verf = node::Buffer::Data(attrs_);
        break;
    default:
        Nan::ThrowTypeError("Invalid creation mode");
    }
}

NFS::Create3Worker::~Create3Worker()
{
    free(error);
    clnt_freeres(client->getClient(), (xdrproc_t) xdr_CREATE3res, (char *) &res);
}

void NFS::Create3Worker::Execute()
{
    if (!client->isMounted()) {
        asprintf(&error, NFSC_NOT_MOUNTED);
        return;
    }
    Serialize my(client);
    bool type_error = false;
    CREATE3args args;
    clnt_stat stat;
    args.how.mode = mode;
    switch (args.how.mode) {
    case UNCHECKED:
    case GUARDED:
        args.how.createhow3_u.obj_attributes =
                node_nfsc_sattr3(v8::Local<v8::Object>::Cast(attrs), type_error);
        break;
    case EXCLUSIVE:
        memcpy(&args.how.createhow3_u.verf[0], verf, NFS3_CREATEVERFSIZE);
    }
    args.where.dir = parent_fh;
    args.where.name = *name;
    if (type_error)
        return;
    stat = nfsproc3_create_3(&args, &res, client->getClient());
    if (stat != RPC_SUCCESS) {
        asprintf(&error, "%s", rpc_error(stat));
        return;
    }
    if (res.status != NFS3_OK) {
        asprintf(&error, "%s", nfs3_error(res.status));
        return;
    }
    success = true;
}

void NFS::Create3Worker::HandleOKCallback()
{
    Nan::HandleScope scope;
    if (success) {
        v8::Local<v8::Value> obj_fh;
        if (res.CREATE3res_u.resok.obj.handle_follows) {
            obj_fh = Nan::NewBuffer(res.CREATE3res_u.resok.obj.post_op_fh3_u.handle.data.data_val,
                                res.CREATE3res_u.resok.obj.post_op_fh3_u.handle.data.data_len)
                    .ToLocalChecked();
        } else {
            obj_fh = Nan::Null();
        }
        v8::Local<v8::Object> wcc = Nan::New<v8::Object>();
        v8::Local<v8::Value> before, after;
        if (res.CREATE3res_u.resok.dir_wcc.before.attributes_follow)
            before = node_nfsc_wcc3(res.CREATE3res_u.resok.dir_wcc.before.pre_op_attr_u.attributes);
        else
            before = Nan::Null();
        if (res.CREATE3res_u.resok.dir_wcc.after.attributes_follow)
            after = node_nfsc_fattr3(res.CREATE3res_u.resok.dir_wcc.after.post_op_attr_u.attributes);
        else
            after = Nan::Null();
        wcc->Set(Nan::New("before").ToLocalChecked(), before);
        wcc->Set(Nan::New("after").ToLocalChecked(), after);

        v8::Local<v8::Value> obj_attrs;
        if (res.CREATE3res_u.resok.obj_attributes.attributes_follow)
            obj_attrs = node_nfsc_fattr3(res.CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes);
        else
            obj_attrs = Nan::Null();

        v8::Local<v8::Value> argv[] = {
            Nan::Null(),
            obj_fh,
            obj_attrs,
            wcc,
        };
        //data stolen by node
        res.CREATE3res_u.resok.obj.post_op_fh3_u.handle.data.data_val = nullptr;
        callback->Call(sizeof(argv)/sizeof(*argv), argv);
    }
    else {
        v8::Local<v8::Object> wcc = Nan::New<v8::Object>();
        v8::Local<v8::Value> before, after;
        if (res.CREATE3res_u.resfail.dir_wcc.before.attributes_follow)
            before = node_nfsc_wcc3(res.CREATE3res_u.resfail.dir_wcc.before.pre_op_attr_u.attributes);
        else
            before = Nan::Null();
        if (res.CREATE3res_u.resfail.dir_wcc.after.attributes_follow)
            after = node_nfsc_fattr3(res.CREATE3res_u.resfail.dir_wcc.after.post_op_attr_u.attributes);
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
}

