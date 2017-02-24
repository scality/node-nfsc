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
#include "node_nfsc_null3.h"
#include "node_nfsc_errors3.h"
#include "node_nfsc_fattr3.h"

// ( cb(err) )
NAN_METHOD(NFS::Client::Null3) {
    bool typeError = true;
    if ( info.Length() != 1) {
        Nan::ThrowTypeError("Must be called with 1 parameters");
        return;
    }
    if (!info[0]->IsFunction())
        Nan::ThrowTypeError("Parameter 1, cb must be a function");
    else
        typeError = false;
    if (typeError)
        return;
    NFS::Client* obj = ObjectWrap::Unwrap<NFS::Client>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[0].As<v8::Function>());
    Nan::AsyncQueueWorker(new NFS::Null3Worker(obj, callback));
}

NFS::Null3Worker::Null3Worker(NFS::Client *client_,
                                Nan::Callback *callback)
    : Nan::AsyncWorker(callback),
      client(client_),
      success(false),
      error(0)
{}

NFS::Null3Worker::~Null3Worker()
{
    free(error);
}

void NFS::Null3Worker::Execute()
{
    if (!client->isMounted()) {
        asprintf(&error, "Not mounted");
        return;
    }
    Serialize my(client);
    clnt_stat stat;
    stat = nfsproc3_null_3(nullptr, nullptr, client->getClient());
    if (stat != RPC_SUCCESS) {
        asprintf(&error, "RPC: getattr failure: %s", rpc_error(stat));
        return;
    }
    success = true;
}

void NFS::Null3Worker::HandleOKCallback()
{
    Nan::HandleScope scope;
    if (success) {
        v8::Local<v8::Value> argv[] = {
            Nan::Null()
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

