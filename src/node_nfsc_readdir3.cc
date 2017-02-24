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
#include <nfsc/libnfs.h>


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
      error(nullptr),
      dir_fh(),
      cookie(),
      cookieverf(),
      count(count_->Uint32Value()),
      res({})
{
    dir_fh.data.data_val = node::Buffer::Data(dir_fh_);
    dir_fh.data.data_len = node::Buffer::Length(dir_fh_);
    if (!cookie_->IsNull()) {
        if (node::Buffer::Length(cookie_) != sizeof(cookie)) {
            Nan::ThrowRangeError("cookie must be 8 bytes long");
            return;
        }
        memcpy(&cookie, node::Buffer::Data(cookie_), sizeof(cookie));
    } else {
        memset(&cookie, 0, sizeof(cookie));
    }
    if (!cookieverf_->IsNull()) {
        if (node::Buffer::Length(cookieverf_) != sizeof(cookieverf)) {
            Nan::ThrowRangeError("cookieverf must be 8 bytes long");
            return;
        }
        memcpy(&cookieverf, node::Buffer::Data(cookieverf_), sizeof(cookieverf));
    } else {
        memset(&cookieverf, 0, sizeof(cookieverf));
    }
}

NFS::ReadDir3Worker::~ReadDir3Worker()
{
    free(error);
    clnt_freeres(client->getClient(), (xdrproc_t) xdr_READDIR3res, (char *) &res);
}

void NFS::ReadDir3Worker::Execute()
{
    if (!client->isMounted()) {
        asprintf(&error, "Not mounted");
        return;
    }
    Serialize my(client);
    READDIR3args args;
    clnt_stat stat;
    args.dir = dir_fh;
    args.cookie = cookie;
    memcpy(args.cookieverf, cookieverf, sizeof(args.cookieverf));
    args.count = count;
    stat = nfsproc3_readdir_3(&args, &res, client->getClient());
    if (stat != RPC_SUCCESS) {
        asprintf(&error, "RPC: readdir failure: %s", rpc_error(stat));
        return;
    }
    if (res.status != NFS3_OK) {
        asprintf(&error, "NFS: readdir failure: %s", nfs3_error(res.status));
        return;
    }
    success = true;

}

void NFS::ReadDir3Worker::HandleOKCallback()
{
    Nan::HandleScope scope;
    if (success) {
        v8::Local<v8::Array> entries = readdir_entries(&res);
        v8::Local<v8::Object> dir_attrs =
                node_nfsc_fattr3(res.READDIR3res_u.resok.dir_attributes.post_op_attr_u.attributes);
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
            Nan::New(error?error:"Unspecified error").ToLocalChecked()
        };
        callback->Call(1, argv);
    }
}
