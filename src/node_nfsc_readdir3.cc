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
#include "node_nfsc_readdir3.h"
#include "node_nfsc_errors3.h"
#include "node_nfsc_fattr3.h"

// (dir, cookie, cookieverf, count, callback(err, dir_attrs, eof, [{ cookie, fileid, name}, ... ]))
NAN_METHOD(NFS::Client::ReadDir3) {
    if ( info.Length() != 5 )
      {
        Nan::ThrowTypeError("Must be called with 5 parameters");
        return;
      }
    bool typeError = true;
    if (!info[0]->IsUint8Array())
        Nan::ThrowTypeError("Parameter 1, dir must be a Buffer");
    if (!info[1]->IsUint8Array() && !info[1]->IsNull())
        Nan::ThrowTypeError("Parameter 2, cookie must be a Buffer or null");
    if (!info[2]->IsUint8Array() && !info[2]->IsNull())
        Nan::ThrowTypeError("Parameter 3, cookieverf must be a Buffer or null");
    if (!info[3]->IsUint32())
        Nan::ThrowTypeError("Parameter 4, count must be a unsigned integer");
    else if (!info[4]->IsFunction())
        Nan::ThrowTypeError("Parameter 5, callback must be a function");
    else
        typeError = false;
    if (typeError)
        return;
    NFS::Client* obj = ObjectWrap::Unwrap<NFS::Client>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[4].As<v8::Function>());
    Nan::AsyncQueueWorker(new NFS::ReadDir3Worker(obj, info[0], info[1], info[2], info[3], callback));
}

static v8::Local<v8::Array>
readdir_entries(READDIR3res *res)
{
    v8::Local<v8::Array> list = Nan::New<v8::Array>();
    int count = 0;
    for (entry3 *entry = res->READDIR3res_u.resok.reply.entries ;
         entry ;
         entry = entry->nextentry ) {
        v8::Local<v8::Object> item = Nan::New<v8::Object>();
        if (entry->name) {
            item->Set(Nan::New("name").ToLocalChecked(),
                      Nan::New(entry->name).ToLocalChecked());
            entry->name = NULL;
        } else {
            item->Set(Nan::New("name").ToLocalChecked(),
                      Nan::Null());
        }
        char *buf_cookie =(char *)malloc(sizeof(cookie3));
        char *buf_fileid =(char *)malloc(sizeof(fileid3));
        memcpy(buf_cookie, &entry->cookie, sizeof(cookie3));
        memcpy(buf_fileid, &entry->fileid, sizeof(fileid3));
        item->Set(Nan::New("cookie").ToLocalChecked(),
                  Nan::NewBuffer(buf_cookie, sizeof(cookie3)).ToLocalChecked());
        item->Set(Nan::New("fileid").ToLocalChecked(),
                  Nan::NewBuffer(buf_fileid, sizeof(fileid3)).ToLocalChecked());
        list->Set(count++, item);

    }
    return list;
}

NFS::ReadDir3Worker::ReadDir3Worker(NFS::Client *client_,
                                  const v8::Local<v8::Value> &dir_fh_,
                                  const v8::Local<v8::Value> &cookie_,
                                  const v8::Local<v8::Value> &cookieverf_,
                                  const v8::Local<v8::Value> &count_,
                                  Nan::Callback *callback)
    : Nan::AsyncWorker(callback),
      client(client_),
      success(false),
      error(NULL),
      res({}),
      args({})
{
    args.dir.data.data_val = node::Buffer::Data(dir_fh_);
    args.dir.data.data_len = node::Buffer::Length(dir_fh_);
    args.count = count_->Uint32Value();
    if (!cookie_->IsNull()) {
        if (node::Buffer::Length(cookie_) != sizeof(args.cookie)) {
            Nan::ThrowRangeError("Invalid cookie size");
            return;
        }
        memcpy(&args.cookie, node::Buffer::Data(cookie_), sizeof(args.cookie));
    } else {
        memset(&args.cookie, 0, sizeof(args.cookie));
    }
    if (!cookieverf_->IsNull()) {
        if (node::Buffer::Length(cookieverf_) != sizeof(args.cookieverf)) {
            Nan::ThrowRangeError("Invalid cookiverf size");
            return;
        }
        memcpy(&args.cookieverf, node::Buffer::Data(cookieverf_), sizeof(args.cookieverf));
    } else {
        memset(&args.cookieverf, 0, sizeof(args.cookieverf));
    }
}

NFS::ReadDir3Worker::~ReadDir3Worker()
{
    Serialize my(client);
    free(error);
    clnt_freeres(client->getClient(), (xdrproc_t) xdr_READDIR3res, (char *) &res);
}

void NFS::ReadDir3Worker::Execute()
{
    if (error)
        return;
    if (!client->isMounted()) {
        NFSC_ASPRINTF(&error, NFSC_NOT_MOUNTED);
        return;
    }
    Serialize my(client);
    clnt_stat stat;
    stat = nfsproc3_readdir_3(&args, &res, client->getClient());
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

void NFS::ReadDir3Worker::HandleOKCallback()
{
    Nan::HandleScope scope;
    if (success) {
        v8::Local<v8::Array> entries = readdir_entries(&res);
        v8::Local<v8::Value> dir_attrs;
        if (res.READDIR3res_u.resok.dir_attributes.attributes_follow)
            dir_attrs = node_nfsc_fattr3(res.READDIR3res_u.resok.dir_attributes.post_op_attr_u.attributes);
        else
            dir_attrs = Nan::Null();
        v8::Local<v8::Value> argv[] = {
            Nan::Null(),
            dir_attrs,
            Nan::New(!!res.READDIR3res_u.resok.reply.eof),
            entries,
        };
        callback->Call(sizeof(argv)/sizeof(*argv), argv);
    }
    else {
        v8::Local<v8::Value> argv[] = {
            Nan::New(error?error:NFSC_UNKNOWN_ERROR).ToLocalChecked()
        };
        callback->Call(1, argv);
    }
}
