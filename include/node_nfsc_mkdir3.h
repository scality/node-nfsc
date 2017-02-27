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
#pragma once
#include <nan.h>
#include "nfs3.h"
#include "node_nfsc_port.h"


namespace NFS {
    class Client;

    class MkDir3Worker : public Nan::AsyncWorker {

        Client *client;
        bool success;
        char *error;
        nfs_fh3 parent_fh;
        Nan::Utf8String name;
        const v8::Local<v8::Value>& attrs;
        MKDIR3res res;
        MKDIR3args args;

    public:

        MkDir3Worker(Client *client_,
                     const v8::Local<v8::Value> &parent_fh_,
                     const v8::Local<v8::Value> &name_,
                      const v8::Local<v8::Value> &attrs_,
                     Nan::Callback *callback);
        ~MkDir3Worker() NFSC_OVERRIDE;
        void Execute() NFSC_OVERRIDE;
        void HandleOKCallback() NFSC_OVERRIDE;

    };
}
