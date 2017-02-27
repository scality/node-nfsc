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


namespace NFS {
    class Client;

    class Remove3Worker : public Nan::AsyncWorker {

        Client *client;
        bool success;
        char *error;
        nfs_fh3 parent_fh;
        Nan::Utf8String name;
        REMOVE3res res;

    public:

        Remove3Worker(Client *client_,
                     const v8::Local<v8::Value> &parent_fh_,
                     const v8::Local<v8::Value> &name_,
                     Nan::Callback *callback);
        ~Remove3Worker() override;
        void Execute() override;
        void HandleOKCallback() override;

    };
}
