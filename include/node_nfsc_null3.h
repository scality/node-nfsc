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

    struct DummyNullArgs {};
    struct DummyNullRes {
        nfsstat3 status;
    };

    class Null3Worker : public Procedure3Worker<DummyNullArgs, DummyNullRes> {

    public:

        Null3Worker(Client *client_,
                     Nan::Callback *callback);

    private:

        clnt_stat xdrProc(DummyNullArgs *a,
                          DummyNullRes *r, CLIENT *c) NFSC_OVERRIDE {
            /* null procedure does nothing, but it still needs a status
             * for the templatized code which relies on it */
            r->status = NFS3_OK;
            return nfsproc3_symlink_3(0, 0, c);
        }
        void procSuccess() NFSC_OVERRIDE;
        void procFailure() NFSC_OVERRIDE;
    };
}
