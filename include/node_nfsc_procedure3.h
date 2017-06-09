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
#include "node_nfsc_errors3.h"



namespace NFS {
    class Client;
    template<typename PROCEDURE3args, typename PROCEDURE3res>
    class Procedure3Worker : public Nan::AsyncWorker {

    protected:
        Client *client;
        bool success;
        char *error;
        xdrproc_t freeFunc;
        PROCEDURE3args args;
        PROCEDURE3res res;

        virtual clnt_stat xdrProc(PROCEDURE3args *,
                                  PROCEDURE3res *,
                                  CLIENT *) = 0;
        virtual void procSuccess() = 0;
        virtual void procFailure() = 0;

    public:

        Procedure3Worker(Client *client_,
                         xdrproc_t freeFunc_,
                         Nan::Callback *callback)
            : Nan::AsyncWorker(callback),
              client(client_),
              success(false),
              error(0),
              freeFunc(freeFunc_),
              args({}),
              res({})
        {}

        ~Procedure3Worker() NFSC_OVERRIDE {
            free(error);
            if (freeFunc) {
                Serialize my(client);
                clnt_freeres(client->getClient(), freeFunc, (char*)&res);
            }
        }
        void Execute() NFSC_OVERRIDE {
            if (!client->isMounted()) {
                NFSC_ASPRINTF(&error, NFSC_NOT_MOUNTED);
                return;
            }
            Serialize my(client);
            clnt_stat stat;
            stat = xdrProc(&args, &res, client->getClient());
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
        void HandleOKCallback() NFSC_OVERRIDE {
            Nan::HandleScope scope;
            if (success) {
                return procSuccess();
            } else {
                return procFailure();
            }
        }
    };
}
