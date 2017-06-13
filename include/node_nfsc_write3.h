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
#include "node_nfsc_procedure3.h"


namespace NFS {
    class Client;

    class Write3Worker : public Procedure3Worker<WRITE3args, WRITE3res> {

    public:

        Write3Worker(Client *client_,
                     const v8::Local<v8::Value> &obj_fh_,
                     const v8::Local<v8::Value> &count_,
                     const v8::Local<v8::Value> &offset_,
                     const v8::Local<v8::Value> &stable_,
                     const v8::Local<v8::Value> &data_,
                     Nan::Callback *callback);

    private:

        clnt_stat xdrProc(WRITE3args *a, WRITE3res *r, CLIENT *c) NFSC_OVERRIDE {
            return nfsproc3_write_3(a, r, c);
        }
        void procSuccess() NFSC_OVERRIDE;
        void procFailure() NFSC_OVERRIDE;
    };
}
