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
#include "node_nfsc_mount3.h"
#include "node_nfsc_lookup3.h"
#include "node_nfsc_getattr3.h"
#include "node_nfsc_readdir3.h"
#include "node_nfsc_readdirplus3.h"
#include "node_nfsc_read3.h"
#include "node_nfsc_access3.h"
#include <gssrpc/rpc.h>
#include "mount3.h"
#include "nfs3.h"

NAN_MODULE_INIT(NFS::Client::Init) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("Client").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    SetPrototypeMethod(tpl, "null3", Null);
    SetPrototypeMethod(tpl, "mount3", Mount);
    SetPrototypeMethod(tpl, "lookup3", Lookup);
    SetPrototypeMethod(tpl, "getattr3", GetAttr);
    SetPrototypeMethod(tpl, "readdir3", ReadDir);
    SetPrototypeMethod(tpl, "readdirplus3", ReadDirPlus);
    SetPrototypeMethod(tpl, "access3", Access);
    SetPrototypeMethod(tpl, "read3", Read);

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

// (cb(err) )
NAN_METHOD(NFS::Client::Null) {
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

// ( cb(err, root_fh) )
NAN_METHOD(NFS::Client::Mount) {
    bool typeError = true;
    if ( info.Length() != 1 ) {
        Nan::ThrowTypeError("Must be called with 1 parameters");
        return;
      }
    if (!info[0]->IsFunction()) {
        Nan::ThrowTypeError("Parameter 1, cb must be a function");
    }
    else
        typeError = false;
    if (typeError)
        return;
    NFS::Client* obj = ObjectWrap::Unwrap<NFS::Client>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[0].As<v8::Function>());
    Nan::AsyncQueueWorker(new NFS::Mount3Worker(obj, callback));
}

// (parent_fh, name, cb(err, obj_fh, obj_attr, dir_attr) )
NAN_METHOD(NFS::Client::Lookup) {
    bool typeError = true;
    if ( info.Length() != 3) {
        Nan::ThrowTypeError("Must be called with 3 parameters");
        return;
    }
    if (!info[0]->IsUint8Array())
        Nan::ThrowTypeError("Parameter 1, parent_fh must be a Buffer");
    else if (!info[1]->IsString())
        Nan::ThrowTypeError("Parameter 2, name must be a string");
    else if (!info[2]->IsFunction())
        Nan::ThrowTypeError("Parameter 3, cb must be a function");
    else
        typeError = false;
    if (typeError)
        return;
    NFS::Client* obj = ObjectWrap::Unwrap<NFS::Client>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[2].As<v8::Function>());
    Nan::AsyncQueueWorker(new NFS::Lookup3Worker(obj, info[0], info[1], callback));
}

// (obj_fh,cb(err, obj_attr) )
NAN_METHOD(NFS::Client::GetAttr) {
    bool typeError = true;
    if ( info.Length() != 2) {
        Nan::ThrowTypeError("Must be called with 2 parameters");
        return;
    }
    if (!info[0]->IsUint8Array())
        Nan::ThrowTypeError("Parameter 1, obj_fh must be a Buffer");
    else if (!info[1]->IsFunction())
        Nan::ThrowTypeError("Parameter 2, cb must be a function");
    else
        typeError = false;
    if (typeError)
        return;
    NFS::Client* obj = ObjectWrap::Unwrap<NFS::Client>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[1].As<v8::Function>());
    Nan::AsyncQueueWorker(new NFS::GetAttr3Worker(obj, info[0], callback));
}

// (dir_fh, cookie, cookieverf, count, cb(err, dir_attrs, eof, [{ cookie, fileid, name}, ... ]))
NAN_METHOD(NFS::Client::ReadDir) {
    if ( info.Length() != 5 )
      {
        Nan::ThrowTypeError("Must be called with 5 parameters");
        return;
      }
    bool typeError = true;
    if (!info[0]->IsUint8Array())
        Nan::ThrowTypeError("Parameter 1, dir_fh must be a Buffer");
    if (!info[1]->IsUint8Array() && !info[1]->IsNull())
        Nan::ThrowTypeError("Parameter 2, cookie must be a Buffer or null");
    if (!info[2]->IsUint8Array() && !info[2]->IsNull())
        Nan::ThrowTypeError("Parameter 3, cookieverf must be a Buffer or null");
    if (!info[3]->IsUint32())
        Nan::ThrowTypeError("Parameter 4, count must be a unsigned integer");
    else if (!info[4]->IsFunction())
        Nan::ThrowTypeError("Parameter 5, cb must be a function");
    else
        typeError = false;
    if (typeError)
        return;
    NFS::Client* obj = ObjectWrap::Unwrap<NFS::Client>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[4].As<v8::Function>());
    Nan::AsyncQueueWorker(new NFS::ReadDir3Worker(obj, info[0], info[1], info[2], info[3], callback));
}

// (dir_fh, cookie, cookieverf, dircount, maxcount, cb(err, dir_attrs, eof, [{ handle, attrs, cookie, fileid, name}, ... ]))
NAN_METHOD(NFS::Client::ReadDirPlus) {
    if ( info.Length() != 6 )
      {
        Nan::ThrowTypeError("Must be called with 6 parameters");
        return;
      }
    bool typeError = true;
    if (!info[0]->IsUint8Array())
        Nan::ThrowTypeError("Parameter 1, dir_fh must be a Buffer");
    if (!info[1]->IsUint8Array() && !info[1]->IsNull())
        Nan::ThrowTypeError("Parameter 2, cookie must be a Buffer or null");
    if (!info[2]->IsUint8Array() && !info[2]->IsNull())
        Nan::ThrowTypeError("Parameter 3, cookieverf must be a Buffer or null");
    if (!info[3]->IsUint32())
        Nan::ThrowTypeError("Parameter 4, dircount must be a unsigned integer");
    if (!info[4]->IsUint32())
        Nan::ThrowTypeError("Parameter 5, maxcount must be a unsigned integer");
    else if (!info[5]->IsFunction())
        Nan::ThrowTypeError("Parameter 6, cb must be a function");
    else
        typeError = false;
    if (typeError)
        return;
    NFS::Client* obj = ObjectWrap::Unwrap<NFS::Client>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[5].As<v8::Function>());
    Nan::AsyncQueueWorker(new NFS::ReadDirPlus3Worker(obj, info[0], info[1], info[2],
            info[3], info[4], callback));
}

// (obj_fh, access, cb(err, access, obj_attr) )
NAN_METHOD(NFS::Client::Access) {
    bool typeError = true;
    if ( info.Length() != 3) {
        Nan::ThrowTypeError("Must be called with 3 parameters");
        return;
    }
    if (!info[0]->IsUint8Array())
        Nan::ThrowTypeError("Parameter 1, obj_fh must be a Buffer");
    else if (!info[1]->IsUint32())
        Nan::ThrowTypeError("Parameter 2, access must be a unsigned integer");
    else if (!info[2]->IsFunction())
        Nan::ThrowTypeError("Parameter 3, cb must be a function");
    else
        typeError = false;
    if (typeError)
        return;
    NFS::Client* obj = ObjectWrap::Unwrap<NFS::Client>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[2].As<v8::Function>());
    Nan::AsyncQueueWorker(new NFS::Access3Worker(obj, info[0], info[1], callback));
}

// (obj_fh, count, offset, cb(err, eof, buf, obj_attr) )
NAN_METHOD(NFS::Client::Read) {
    bool typeError = true;
    if ( info.Length() != 4) {
        Nan::ThrowTypeError("Must be called with 4 parameters");
        return;
    }
    if (!info[0]->IsUint8Array())
        Nan::ThrowTypeError("Parameter 1, obj_fh must be a Buffer");
    else if (!info[1]->IsNumber())
        Nan::ThrowTypeError("Parameter 2, count must be a unsigned integer");
    else if (!info[2]->IsNumber())
        Nan::ThrowTypeError("Parameter 3, offset must be a unsigned integer");
    else if (!info[3]->IsFunction())
        Nan::ThrowTypeError("Parameter 4, cb must be a function");
    else
        typeError = false;
    if (typeError)
        return;
    NFS::Client* obj = ObjectWrap::Unwrap<NFS::Client>(info.Holder());
    Nan::Callback *callback = new Nan::Callback(info[3].As<v8::Function>());
    Nan::AsyncQueueWorker(new NFS::Read3Worker(obj, info[0], info[1], info[2], callback));
}

NODE_MODULE(NFS, NFS::Client::Init)
