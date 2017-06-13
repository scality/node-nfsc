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
#include "node_nfsc_rename3.h"
#include "node_nfsc_fattr3.h"
#include "node_nfsc_wcc3.h"

// (from_dir, from_name, to_dir, to_name, callback(err, fromdir_wcc, todir_wcc) )
NAN_METHOD(NFS::Client::Rename3) {
    bool typeError = true;
    if ( info.Length() != 5) {
        Nan::ThrowTypeError("Must be called with 5 parameters");
        return;
    }
    if (!info[0]->IsUint8Array())
        Nan::ThrowTypeError("Parameter 1, from_dir must be a Buffer");
    else if (!info[1]->IsString())
        Nan::ThrowTypeError("Parameter 2, from_name must be a string");
    else if (!info[2]->IsUint8Array())
        Nan::ThrowTypeError("Parameter 3, to_dir must be a Buffer");
    else if (!info[3]->IsString())
        Nan::ThrowTypeError("Parameter 4, to_name must be a string");
    else if (!info[4]->IsFunction())
        Nan::ThrowTypeError("Parameter 5, callback must be a function");
    else
        typeError = false;
    if (typeError)
        return;
    NFS::Client* obj = ObjectWrap::Unwrap<NFS::Client>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[4].As<v8::Function>());
    Nan::AsyncQueueWorker(new NFS::Rename3Worker(obj, info[0], info[1],
                                                 info[2], info[3], callback));
}

NFS::Rename3Worker::Rename3Worker(NFS::Client *client_,
                                const v8::Local<v8::Value> &from_fh_,
                                const v8::Local<v8::Value> &from_name_,
                                const v8::Local<v8::Value> &to_fh_,
                                const v8::Local<v8::Value> &to_name_,
                                Nan::Callback *callback)
    : Procedure3Worker(client_, (xdrproc_t) xdr_RENAME3res, callback),
      from_name(from_name_),
      to_name(to_name_)
{
    args.from.dir.data.data_val = node::Buffer::Data(from_fh_);
    args.from.dir.data.data_len = node::Buffer::Length(from_fh_);
    args.to.dir.data.data_val = node::Buffer::Data(to_fh_);
    args.to.dir.data.data_len = node::Buffer::Length(to_fh_);
    args.from.name = *from_name;
    args.to.name = *to_name;
}

void NFS::Rename3Worker::procSuccess()
{
    v8::Local<v8::Object> from_wcc = Nan::New<v8::Object>();
    v8::Local<v8::Value> from_before, from_after;
    if (res.RENAME3res_u.resok.fromdir_wcc.before.attributes_follow)
        from_before = node_nfsc_wcc3(res.RENAME3res_u.resok.fromdir_wcc
                                     .before.pre_op_attr_u.attributes);
    else
        from_before = Nan::Null();
    if (res.RENAME3res_u.resok.fromdir_wcc.after.attributes_follow)
        from_after = node_nfsc_fattr3(res.RENAME3res_u.resok.fromdir_wcc
                                      .after.post_op_attr_u.attributes);
    else
        from_after = Nan::Null();
    from_wcc->Set(Nan::New("before").ToLocalChecked(), from_before);
    from_wcc->Set(Nan::New("after").ToLocalChecked(), from_after);

    v8::Local<v8::Object> to_wcc = Nan::New<v8::Object>();
    v8::Local<v8::Value> to_before, to_after;
    if (res.RENAME3res_u.resok.todir_wcc.before.attributes_follow)
        to_before = node_nfsc_wcc3(res.RENAME3res_u.resok.todir_wcc
                                   .before.pre_op_attr_u.attributes);
    else
        to_before = Nan::Null();
    if (res.RENAME3res_u.resok.todir_wcc.after.attributes_follow)
        to_after = node_nfsc_fattr3(res.RENAME3res_u.resok.todir_wcc
                                    .after.post_op_attr_u.attributes);
    else
        to_after = Nan::Null();
    to_wcc->Set(Nan::New("before").ToLocalChecked(), to_before);
    to_wcc->Set(Nan::New("after").ToLocalChecked(), to_after);

    v8::Local<v8::Value> argv[] = {
        Nan::Null(),
        from_wcc,
        to_wcc,
    };
    callback->Call(sizeof(argv)/sizeof(*argv), argv);
}

void NFS::Rename3Worker::procFailure()
{
    v8::Local<v8::Object> from_wcc = Nan::New<v8::Object>();
    v8::Local<v8::Value> from_before, from_after;
    if (res.RENAME3res_u.resfail.fromdir_wcc.before.attributes_follow)
        from_before = node_nfsc_wcc3(res.RENAME3res_u.resfail.fromdir_wcc
                                     .before.pre_op_attr_u.attributes);
    else
        from_before = Nan::Null();
    if (res.RENAME3res_u.resfail.fromdir_wcc.after.attributes_follow)
        from_after = node_nfsc_fattr3(res.RENAME3res_u.resfail.fromdir_wcc
                                      .after.post_op_attr_u.attributes);
    else
        from_after = Nan::Null();
    from_wcc->Set(Nan::New("before").ToLocalChecked(), from_before);
    from_wcc->Set(Nan::New("after").ToLocalChecked(), from_after);

    v8::Local<v8::Object> to_wcc = Nan::New<v8::Object>();
    v8::Local<v8::Value> to_before, to_after;
    if (res.RENAME3res_u.resfail.todir_wcc.before.attributes_follow)
        to_before = node_nfsc_wcc3(res.RENAME3res_u.resfail.todir_wcc
                                   .before.pre_op_attr_u.attributes);
    else
        to_before = Nan::Null();
    if (res.RENAME3res_u.resfail.todir_wcc.after.attributes_follow)
        to_after = node_nfsc_fattr3(res.RENAME3res_u.resfail.todir_wcc
                                    .after.post_op_attr_u.attributes);
    else
        to_after = Nan::Null();
    to_wcc->Set(Nan::New("before").ToLocalChecked(), to_before);
    to_wcc->Set(Nan::New("after").ToLocalChecked(), to_after);

    v8::Local<v8::Value> argv[] = {
        Nan::New(error?error:NFSC_UNKNOWN_ERROR).ToLocalChecked(),
        from_wcc,
        to_wcc,
    };
    callback->Call(3, argv);
}
