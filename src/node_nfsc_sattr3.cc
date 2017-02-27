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
#include "node_nfsc_sattr3.h"
#include "node_nfsc.h"
sattr3
node_nfsc_sattr3(const v8::Local<v8::Object>& attr)
{
    sattr3 res = {};
    res.atime.set_it = DONT_CHANGE;
    res.mtime.set_it = DONT_CHANGE;
    v8::Local<v8::Value> atime = attr->Get(Nan::New("atime").ToLocalChecked());
    v8::Local<v8::Value> mtime = attr->Get(Nan::New("mtime").ToLocalChecked());
    v8::Local<v8::Value> size = attr->Get(Nan::New("size").ToLocalChecked());
    v8::Local<v8::Value> mode = attr->Get(Nan::New("mode").ToLocalChecked());
    v8::Local<v8::Value> uid = attr->Get(Nan::New("uid").ToLocalChecked());
    v8::Local<v8::Value> gid = attr->Get(Nan::New("gid").ToLocalChecked());
    if (atime->IsInt32()) {
        res.atime.set_it = SET_TO_CLIENT_TIME;
        v8::Local<v8::Value> atime_nsec = attr->Get(Nan::New("atime_nsec")
                                                    .ToLocalChecked());
        res.atime.set_atime_u.atime.seconds = atime->Int32Value();
        if (atime_nsec->IsInt32()) {
            res.atime.set_atime_u.atime.nseconds = atime_nsec->Int32Value();
        } else if (!atime_nsec->IsUndefined()) {
             Nan::ThrowTypeError("Invalid atime_nsec");
             return {};
        }
    } else if (atime->IsNull()) {
        res.atime.set_it = SET_TO_SERVER_TIME;
        res.atime.set_atime_u.atime = {};
    } else if (!atime->IsUndefined()) {
        Nan::ThrowTypeError("Invalid atime");
        return {};
    }
    if (mtime->IsInt32()) {
        res.mtime.set_it = SET_TO_CLIENT_TIME;
        v8::Local<v8::Value> mtime_nsec = attr->Get(Nan::New("mtime_nsec")
                                                    .ToLocalChecked());
        res.mtime.set_mtime_u.mtime.seconds = mtime->Int32Value();
        if (mtime_nsec->IsInt32()) {
            res.mtime.set_mtime_u.mtime.nseconds = mtime_nsec->Int32Value();
        } else if (!mtime->IsUndefined()) {
            Nan::ThrowTypeError("Invalid mtime_nsec");
            return {};
        }
    } else if (mtime->IsNull()) {
        res.mtime.set_it = SET_TO_SERVER_TIME;
        res.mtime.set_mtime_u.mtime = {};
    } else if (!mtime->IsUndefined()) {
        Nan::ThrowTypeError("Invalid mtime");
        return {};
    }
    if (size->IsNumber()) {
        res.size.set_it = true;
        double d_size = size->NumberValue();
        size_t size = NFS::CheckUDouble(d_size);
        if (size != (size_t)-1) {
            res.size.set_size3_u.size = size;
        } else {
            Nan::ThrowRangeError("Invalid size");
            return {};
        }
    } else if (!size->IsUndefined()) {
        Nan::ThrowRangeError("Invalid size");
        return {};
    }
    if (mode->IsUint32()) {
        res.mode.set_it = true;
        res.mode.set_mode3_u.mode = mode->Uint32Value();
    } else if (!mode->IsUndefined()) {
        Nan::ThrowRangeError("Invalid mode");
        return {};
    }
    if (uid->IsUint32()) {
        res.uid.set_it = true;
        res.uid.set_uid3_u.uid = uid->Uint32Value();
    } else if (!uid->IsUndefined()) {
        Nan::ThrowRangeError("Invalid uid");
        return {};
    }
    if (gid->IsUint32()) {
        res.gid.set_it = true;
        res.gid.set_gid3_u.gid = gid->Uint32Value();
    } else if (!gid->IsUndefined()) {
        Nan::ThrowRangeError("Invalid gid");
        return {};
    }
    return res;
}
