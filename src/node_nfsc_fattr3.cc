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
#include "node_nfsc_fattr3.h"

static const char*
ftype3_str(ftype3 type)
{
    switch (type) {
    case NF3REG:
        return "NF3REG";
    case NF3DIR:
        return "NF3DIR";
    case NF3BLK:
        return "NF3BLK";
    case NF3CHR:
        return "NF3CHR";
    case NF3LNK:
        return "NF3LNK";
    case NF3SOCK:
        return "NF3SOCK";
    case NF3FIFO:
        return "NF3FIFO";
    default:
        return "UNKNOWN";
    }
}

v8::Local<v8::Object> node_nfsc_fattr3(fattr3 &attr)
{
    char *buf_fileid = (char*)malloc(sizeof(attr.fileid));
    char *buf_fsid = (char*)malloc(sizeof(attr.fsid));
    memcpy(buf_fileid, &attr.fileid, sizeof(attr.fileid));
    memcpy(buf_fsid, &attr.fsid, sizeof(attr.fsid));
    v8::Local<v8::Object> obj = Nan::New<v8::Object>();
    obj->Set(Nan::New("atime").ToLocalChecked(),
             Nan::New<v8::Uint32>(attr.atime.seconds));
    obj->Set(Nan::New("atime_nsec").ToLocalChecked(),
             Nan::New<v8::Uint32>(attr.atime.nseconds));
    obj->Set(Nan::New("ctime").ToLocalChecked(),
             Nan::New<v8::Uint32>(attr.ctime.seconds));
    obj->Set(Nan::New("ctime_nsec").ToLocalChecked(),
             Nan::New<v8::Uint32>(attr.ctime.nseconds));
    obj->Set(Nan::New("mtime").ToLocalChecked(),
             Nan::New<v8::Uint32>(attr.mtime.seconds));
    obj->Set(Nan::New("mtime_nsec").ToLocalChecked(),
             Nan::New<v8::Uint32>(attr.mtime.nseconds));
    obj->Set(Nan::New("fileid").ToLocalChecked(),
             Nan::NewBuffer(buf_fileid,
                            sizeof(attr.fileid))
             .ToLocalChecked());
    obj->Set(Nan::New("fsid").ToLocalChecked(),
             Nan::NewBuffer(buf_fsid,
                            sizeof(attr.fsid))
             .ToLocalChecked());
    obj->Set(Nan::New("uid").ToLocalChecked(),
             Nan::New(attr.uid));
    obj->Set(Nan::New("gid").ToLocalChecked(),
             Nan::New(attr.gid));
    obj->Set(Nan::New("mode").ToLocalChecked(),
             Nan::New(attr.mode));
    obj->Set(Nan::New("nlink").ToLocalChecked(),
             Nan::New(attr.nlink));
/*
    obj->Set(Nan::New("rdev").ToLocalChecked(),
             Nan::New(attr.rdev).ToLocalChecked());
*/
    obj->Set(Nan::New("size").ToLocalChecked(),
             Nan::New(double(attr.size)));
    obj->Set(Nan::New("used").ToLocalChecked(),
             Nan::New(double(attr.used)));
    obj->Set(Nan::New("type").ToLocalChecked(),
             Nan::New(ftype3_str(attr.type)).ToLocalChecked());
    return obj;
}
