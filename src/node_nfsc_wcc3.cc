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
#include "node_nfsc_wcc3.h"

v8::Local<v8::Object> node_nfsc_wcc3(const wcc_attr& attr)
{
    v8::Local<v8::Object> obj = Nan::New<v8::Object>();
    obj->Set(Nan::New("ctime").ToLocalChecked(),
             Nan::New<v8::Uint32>(attr.ctime.seconds));
    obj->Set(Nan::New("ctime_nsec").ToLocalChecked(),
             Nan::New<v8::Uint32>(attr.ctime.nseconds));
    obj->Set(Nan::New("mtime").ToLocalChecked(),
             Nan::New<v8::Uint32>(attr.mtime.seconds));
    obj->Set(Nan::New("mtime_nsec").ToLocalChecked(),
             Nan::New<v8::Uint32>(attr.mtime.nseconds));
    obj->Set(Nan::New("size").ToLocalChecked(),
             Nan::New(double(attr.size)));
    return obj;
}
