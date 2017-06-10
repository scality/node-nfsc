'use strict';
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

class NFSError extends Error {

    constructor(status, code, desc, info) {
        super(status);
        this.status = status;
        this.code = code;
        this.description = desc;
        Object.keys(info || {})
            .forEach(index => {
                this[index] = info[index];
            });
    }

    newInstance(info) {
        return new NFSError(this.status, this.code, this.description, info);
    }

}

class ErrorFactory {

    constructor(errorsDb) {
        const errors = require(errorsDb);
        this.errorTemplates = {};

        Object.keys(errors)
            .forEach(index => {
                this.errorTemplates[index] =
                    new NFSError(index, errors[index].code,
                                   errors[index].description);
            });
        this.unknownError = new NFSError('UNKNOWN', -1, 'Unknown error.');
    }
    create(type, info) {
        const template = this.errorTemplates[type] || this.unknownError;
        return template.newInstance(info);
    }
}

module.exports = ErrorFactory;
