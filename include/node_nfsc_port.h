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

#if __cplusplus < 201103L
# define NFSC_OVERRIDE
#else
# define NFSC_OVERRIDE override
#endif

#define NFSC_ASPRINTF(__strp__, __fmt__, ...) \
    do {\
        char **p = (__strp__);\
        int ret = asprintf(p, __fmt__, ##__VA_ARGS__);\
        if ( ret < 0 )\
            *p = NULL;\
    } while (0)
