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
#include <gssrpc/rpc.h>
#include "mount3.h"
#include "nfs3.h"

NAN_MODULE_INIT(NFS::Client::Init) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("Client").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    SetPrototypeMethod(tpl, "null3", Null3);
    SetPrototypeMethod(tpl, "mount3", Mount3);
    SetPrototypeMethod(tpl, "lookup3", Lookup3);
    SetPrototypeMethod(tpl, "getattr3", GetAttr3);
    SetPrototypeMethod(tpl, "readdir3", ReadDir3);
    SetPrototypeMethod(tpl, "readdirplus3", ReadDirPlus3);
    SetPrototypeMethod(tpl, "access3", Access3);
    SetPrototypeMethod(tpl, "read3", Read3);
    SetPrototypeMethod(tpl, "write3", Write3);
    SetPrototypeMethod(tpl, "commit3", Commit3);
    SetPrototypeMethod(tpl, "create3", Create3);

    constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("Client").ToLocalChecked(),
        Nan::GetFunction(tpl).ToLocalChecked());
}



NFS::Serialize::Serialize(NFS::Client *client_)
    : client(client_) {
    sem_wait(&client->sem);
}



NFS::Serialize::~Serialize() {
    sem_post(&client->sem);
}


CLIENT *NFS::Client::getClient()
{
    return client;
}

const char *NFS::Client::getHost() const
{
    return *host;
}

const char *NFS::Client::getExportPath() const
{
    return *exportPath;
}

void NFS::Client::setClient(CLIENT *client_)
{
    client = client_;
}

void NFS::Client::setMounted(bool v)
{
    mounted = v;
}


nfs_fh3 &NFS::Client::getRootFh()
{
    return *rootFh;
}

void NFS::Client::freeRootFh()
{
    if (rootFh) {
        delete[] rootFh->data.data_val;
        delete rootFh;
    }
}

void NFS::Client::setRootFh(char *data, size_t len)
{
    freeRootFh();
    rootFh = new nfs_fh3;
    rootFh->data.data_val = new char[len];
    rootFh->data.data_len = len;
    memcpy(rootFh->data.data_val, data, len);
}

const char *NFS::Client::getProtocol() const
{
    return *protocol;
}

const char *NFS::Client::getAuthenticationMethod() const
{
    return *authenticationMethod;
}

int NFS::Client::getUid() const
{
    return uid;
}

int NFS::Client::getGid() const
{
    return gid;
}

timeval &NFS::Client::getTimeout()
{
    return timeout;
}

bool NFS::Client::isMounted() const
{
    return mounted;
}

NFS::Client::Client(const v8::Local<v8::Value> &host_,
                    const v8::Local<v8::Value> &exportPath_,
                    const v8::Local<v8::Value> &protocol_,
                    const v8::Local<v8::Value> &uid_,
                    const v8::Local<v8::Value> &gid_,
                    const v8::Local<v8::Value> &authenticationMethod_,
                    const v8::Local<v8::Value> &timeout_) :
    sem(),
    client(nullptr),
    mounted(false),
    rootFh(nullptr),
    host(host_),
    exportPath(exportPath_),
    protocol(protocol_),
    uid(uid_->Int32Value()),
    gid(gid_->Int32Value()),
    authenticationMethod(authenticationMethod_),
    timeout({timeout_->Int32Value(), 0})
{
    sem_init(&sem, 0, 1);
}

NFS::Client::~Client()
{
    if (client) {
        if (client->cl_auth)
            auth_destroy(client->cl_auth);
        clnt_destroy(client);
    }
}

NAN_METHOD(NFS::Client::New) {
    bool typeError = true;
    if (info.Length() != 7) {
        Nan::ThrowSyntaxError("Must be called with 7 parameters");
        return;
    }
    if (!info[0]->IsString())
        Nan::ThrowTypeError("Parameter 1, host must be a string");
    else if (!info[1]->IsString())
        Nan::ThrowTypeError("Parameter 2, exportPath must be a string");
    else if (!info[2]->IsString())
        Nan::ThrowTypeError("Parameter 3, protocol must be a string");
    else if (!info[3]->IsInt32())
        Nan::ThrowTypeError("Parameter 4, uid must be a signed integer");
    else if (!info[4]->IsInt32())
        Nan::ThrowTypeError("Parameter 5, gid must be a signed integer");
    else if (!info[5]->IsString())
        Nan::ThrowTypeError("Parameter 6, authenticationMethod"
                            " must be a String");
    else if (!info[6]->IsInt32())
        Nan::ThrowTypeError("Parameter 7, timeout must be a signed integer");
    else
        typeError = false;
    if (typeError)
        return;

    if ( info.IsConstructCall()) {
        NFS::Client *obj = new NFS::Client(info[0], info[1],
                info[2], info[3],
                info[4], info[5],
                info[6]);
        obj->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
    } else {
        const int argc = 7;
        v8::Local<v8::Value> argv[] = {
            info[0],
            info[1],
            info[2],
            info[3],
            info[4],
            info[5],
            info[6]
        };
        v8::Local<v8::Function> cons = Nan::New(constructor());
        info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv)
                                  .ToLocalChecked());
    }
}

NODE_MODULE(NFS, NFS::Client::Init)
