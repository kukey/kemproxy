/*
 * kemproxy - A fast and lightweight proxy for memcached protocol.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _NC_SLOWLOG_H_
#define _NC_SLOWLOG_H_

#include <nc_core.h>

struct slowlog {
    TAILQ_ENTRY(slowlog) slowlog_tqe;
    uint64_t             id;
    uint64_t             duration;
    time_t               time;
    struct string        iport;
    struct string        req_type;
    struct string        key0;
    uint32_t             req_nargs;
    struct string        slowlog_str;
};

struct slowlog *slowlog_get(void);
void slowlog_put(struct slowlog *slog);
void slowlog_insert(struct msg *m, struct slowlog *slog, 
        uint64_t duration, time_t time, const char *iport,
        struct string *req_type, struct keypos *key0);
void slowlog_tq_deinit(struct server_pool *sp);
int slowlog_empty(struct server_pool *sp);

#endif
