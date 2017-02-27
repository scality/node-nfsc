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

#include <semaphore.h>
#include <nan.h>
#include "mount3.h"
#include "nfs3.h"

#define NFSC_NOT_MOUNTED "NFSC_NOT_MOUNTED"
#define NFSC_ALREADY_MOUNTED "NFSC_ALREADY_MOUNTED"
#define NFSC_ERANGE "NFSC_ERANGE"
#define NFSC_UNKNOWN_ERROR "NFSC_UNKNOWN_ERROR"
#define NFSC_EGETHOSTNAME "NFSC_EGETHOSTNAME"
#define NFSC_EGETHOSTBYNAME "NFSC_EGETHOSTBYNAME"
namespace NFS {
class Client;
class Serialize {
    Client *client;
public:
    Serialize(Client *client_);
    ~Serialize();
};

static inline uint64_t
CheckUDouble(double v) {
    if ( v < 0  || v >= ((1LL<<53)-1) )
        return (uint64_t)-1;
    return (uint64_t)v;
}


class Client : public Nan::ObjectWrap {
    friend class Serialize;
public:

    static NAN_MODULE_INIT(Init);

    CLIENT *getClient();
    void setClient(CLIENT *client_);
    nfs_fh3 &getRootFh();
    void freeRootFh();
    void setRootFh(char *data, size_t len);
    const char* getHost() const;
    const char* getExportPath() const;
    const char* getProtocol() const;
    const char* getAuthenticationMethod() const;
    int getUid() const;
    int getGid() const;
    timeval& getTimeout();
    bool isMounted() const;
    void setMounted(bool v = true);

private:
    sem_t sem;
    CLIENT *client;
    bool mounted;
    nfs_fh3 *rootFh;
    Nan::Utf8String host;
    Nan::Utf8String exportPath;
    Nan::Utf8String protocol;
    int uid;
    int gid;
    Nan::Utf8String authenticationMethod;
    timeval timeout;

    Client(const v8::Local<v8::Value> &host_,
           const v8::Local<v8::Value> &exportPath_,
           const v8::Local<v8::Value> &protocol_,
           const v8::Local<v8::Value> &uid_,
           const v8::Local<v8::Value> &gid_,
           const v8::Local<v8::Value> &authenticationMethod_,
           const v8::Local<v8::Value> &timeout_);
    ~Client() override;

    static NAN_METHOD(New);

    /* NFSv3 RPCs */
    static NAN_METHOD(Null3);
    static NAN_METHOD(Mount3);
    static NAN_METHOD(Lookup3);
    static NAN_METHOD(GetAttr3);
    static NAN_METHOD(ReadDir3);
    static NAN_METHOD(ReadDirPlus3);
    static NAN_METHOD(Access3);
    static NAN_METHOD(Read3);
    static NAN_METHOD(Write3);
    static NAN_METHOD(Commit3);
    static NAN_METHOD(Create3);
    static NAN_METHOD(Remove3);
    static NAN_METHOD(RmDir3);
    static NAN_METHOD(MkDir3);
    static NAN_METHOD(SetAttr3);

    /*
    static NAN_METHOD(ReadLink);
    static NAN_METHOD(FsStat);
    static NAN_METHOD(FsInfo);
    static NAN_METHOD(PathConf);

    static NAN_METHOD(Rename);

    static NAN_METHOD(SymLink);
    static NAN_METHOD(MkNod);
    static NAN_METHOD(Link);
    */
    static inline Nan::Persistent<v8::Function> & constructor() {
        static Nan::Persistent<v8::Function> my_constructor;
        return my_constructor;
    }
};
}
