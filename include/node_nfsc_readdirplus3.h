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

    class ReadDirPlus3Worker : public Nan::AsyncWorker {

        Client *client;
        bool success;
        char *error;
        nfs_fh3 dir_fh;
        uint64_t cookie;
        char cookieverf[NFS3_COOKIEVERFSIZE];
        unsigned int dircount;
        unsigned int maxcount;
        READDIRPLUS3res res;

    public:

        ReadDirPlus3Worker(Client *client_,
                           const v8::Local<v8::Value> &dir_fh_,
                           const v8::Local<v8::Value> &cookie_,
                           const v8::Local<v8::Value> &cookieverf_,
                           const v8::Local<v8::Value> &dircount_,
                           const v8::Local<v8::Value> &maxcount_,
                           Nan::Callback *callback);
        ~ReadDirPlus3Worker() NFSC_OVERRIDE;
        void Execute() NFSC_OVERRIDE;
        void HandleOKCallback() NFSC_OVERRIDE;
    };
}
