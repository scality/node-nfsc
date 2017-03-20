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
#include "node_nfsc_setattr3.h"
#include "node_nfsc_errors3.h"
#include "node_nfsc_sattr3.h"
#include "node_nfsc_fattr3.h"
#include "node_nfsc_wcc3.h"

// (object, attrs, guard, callback(err, wcc) )
NAN_METHOD(NFS::Client::SetAttr3) {
    bool typeError = true;
    if ( info.Length() != 4) {
        Nan::ThrowTypeError("Must be called with 4 parameters");
        return;
    }
    if (!info[0]->IsUint8Array())
        Nan::ThrowTypeError("Parameter 1, object must be a Buffer");
    else if (!info[1]->IsObject())
        Nan::ThrowTypeError("Parameter 2, attr must be an Object");
    else if (!info[2]->IsObject() && !info[2]->IsNull())
        Nan::ThrowTypeError("Parameter 3, guard must be an Object or null");
    else if (!info[3]->IsFunction())
        Nan::ThrowTypeError("Parameter 4, callback must be a function");
    else
        typeError = false;
    if (typeError)
        return;
    NFS::Client* obj = ObjectWrap::Unwrap<NFS::Client>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[3].As<v8::Function>());
    Nan::AsyncQueueWorker(new NFS::SetAttr3Worker(obj, info[0], info[1],
                                                 info[2], callback));
}

NFS::SetAttr3Worker::SetAttr3Worker(NFS::Client *client_,
                                    const v8::Local<v8::Value> &obj_fh_,
                                    const v8::Local<v8::Value> &attrs_,
                                    const v8::Local<v8::Value> &guard_,
                                    Nan::Callback *callback)
    : Nan::AsyncWorker(callback),
      client(client_),
      success(false),
      error(0),
      obj_fh(),
      attrs(attrs_),
      guard(guard_),
      res({}),
      args({})
{
    obj_fh.data.data_val = node::Buffer::Data(obj_fh_);
    obj_fh.data.data_len = node::Buffer::Length(obj_fh_);
    if (!guard->IsNull()) {
        v8::Local<v8::Object> ctime = v8::Local<v8::Object>::Cast(guard);
        args.guard.check = 0;
        args.guard.sattrguard3_u.obj_ctime.seconds =
                ctime->Get(Nan::New("ctime")
                           .ToLocalChecked())->Int32Value();
        args.guard.sattrguard3_u.obj_ctime.nseconds =
                ctime->Get(Nan::New("ctime_nsec")
                           .ToLocalChecked())->Int32Value();
    } else {
        args.guard.check = 0;
        args.guard.sattrguard3_u.obj_ctime = {};
    }
    args.object = obj_fh;
    args.new_attributes = node_nfsc_sattr3(v8::Local<v8::Object>::Cast(attrs));
}

NFS::SetAttr3Worker::~SetAttr3Worker()
{
    Serialize my(client);
    free(error);
    clnt_freeres(client->getClient(), (xdrproc_t) xdr_SETATTR3res, (char *) &res);
}

void NFS::SetAttr3Worker::Execute()
{
    if (!client->isMounted()) {
        NFSC_ASPRINTF(&error, NFSC_NOT_MOUNTED);
        return;
    }
    Serialize my(client);
    clnt_stat stat;
    stat = nfsproc3_setattr_3(&args, &res, client->getClient());
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

void NFS::SetAttr3Worker::HandleOKCallback()
{
    Nan::HandleScope scope;
    if (success) {
        v8::Local<v8::Value> obj_fh;
        v8::Local<v8::Object> wcc = Nan::New<v8::Object>();
        v8::Local<v8::Value> before, after;
        if (res.SETATTR3res_u.resok.obj_wcc.before.attributes_follow)
            before = node_nfsc_wcc3(res.SETATTR3res_u.resok.obj_wcc.before.pre_op_attr_u.attributes);
        else
            before = Nan::Null();
        if (res.SETATTR3res_u.resok.obj_wcc.after.attributes_follow)
            after = node_nfsc_fattr3(res.SETATTR3res_u.resok.obj_wcc.after.post_op_attr_u.attributes);
        else
            after = Nan::Null();
        wcc->Set(Nan::New("before").ToLocalChecked(), before);
        wcc->Set(Nan::New("after").ToLocalChecked(), after);

        v8::Local<v8::Value> argv[] = {
            Nan::Null(),
            wcc,
        };
        callback->Call(sizeof(argv)/sizeof(*argv), argv);
    }
    else {
        v8::Local<v8::Object> wcc = Nan::New<v8::Object>();
        v8::Local<v8::Value> before, after;
        if (res.SETATTR3res_u.resfail.obj_wcc.before.attributes_follow)
            before = node_nfsc_wcc3(res.SETATTR3res_u.resfail.obj_wcc.before.pre_op_attr_u.attributes);
        else
            before = Nan::Null();
        if (res.SETATTR3res_u.resfail.obj_wcc.after.attributes_follow)
            after = node_nfsc_fattr3(res.SETATTR3res_u.resfail.obj_wcc.after.post_op_attr_u.attributes);
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

