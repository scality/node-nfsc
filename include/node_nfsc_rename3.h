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

    class Rename3Worker : public Nan::AsyncWorker {

        Client *client;
        bool success;
        char *error;
        Nan::Utf8String from_name;
        Nan::Utf8String to_name;
        RENAME3res res;
        RENAME3args args;

    public:

        Rename3Worker(Client *client_,
                      const v8::Local<v8::Value> &from_fh_,
                      const v8::Local<v8::Value> &from_name_,
                      const v8::Local<v8::Value> &to_fh_,
                      const v8::Local<v8::Value> &to_name_,
                      Nan::Callback *callback);
        ~Rename3Worker() NFSC_OVERRIDE;
        void Execute() NFSC_OVERRIDE;
        void HandleOKCallback() NFSC_OVERRIDE;

    };
}
