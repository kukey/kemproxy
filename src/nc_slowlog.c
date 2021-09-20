/*
 * kemproxy - A fast and lightweight proxy for memcached protocol.
 * kemproxy is fork form twitter/twemproxy.
 * 
/*
 * kemproxy - A fast and lightweight proxy for memcached protocol.
 * kemproxy is fork form twitter/twemproxy.
 * 
 * Copyright (C) 2021, wei huang <wei.kukey@gmail.com>
 * All rights reserved.
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

#include <nc_slowlog.h>
#include <nc_util.h>
#include <nc_queue.h>
#include <nc_string.h>
#include <nc_message.h>
#include <nc_server.h>

/* global var */
static uint64_t slowlog_id = 0;

/* function */
struct slowlog *
slowlog_get(void)
{
    struct slowlog *slog = nc_alloc(sizeof(struct slowlog));
    if (slog == NULL) {
        return NULL;
    }

    slog->id = slowlog_id++;
    string_init(&slog->iport);
    string_init(&slog->slowlog_str);
    string_init(&slog->req_type);
    string_init(&slog->key0);

    return slog;
}

void
slowlog_put(struct slowlog *slog)
{
    string_deinit(&slog->iport);
    string_deinit(&slog->slowlog_str);
    string_deinit(&slog->req_type);
    string_deinit(&slog->key0);
    nc_free(slog);
}

void
slowlog_insert(struct msg *m, struct slowlog *slog, 
        uint64_t duration, time_t time, const char *iport,
        struct string *req_type, struct keypos *key0) 
{
    struct server_pool *sp = ((struct conn*)(m->owner))->owner;
    struct slowlog *slast = NULL;

    ASSERT(sp != NULL);

    slog->duration = duration;
    slog->time = time;
    string_copy(&slog->iport, (uint8_t*)iport, (uint32_t)nc_strlen(iport));
    string_copy(&slog->key0, key0->start, (uint32_t)(key0->end - key0->start));
    string_duplicate(&slog->req_type, req_type);
    slog->req_nargs = m->narg;

    if (sp->n_slowlog == sp->max_slowlog) {
        slast = TAILQ_LAST(&sp->slowlog_q, slog_tqh);
        TAILQ_REMOVE(&sp->slowlog_q, slast, slowlog_tqe);
        slowlog_put(slast);
    } else {
        sp->n_slowlog++;
    }   
    TAILQ_INSERT_HEAD(&sp->slowlog_q, slog, slowlog_tqe);
}

void
slowlog_tq_deinit(struct server_pool *sp)
{
    struct slowlog *slog = NULL;

    if (TAILQ_EMPTY(&sp->slowlog_q)) {
        return;
    }

    TAILQ_FOREACH(slog, &sp->slowlog_q, slowlog_tqe) {
        if (slog) {
            slowlog_put(slog);
        }
    }
}

int slowlog_empty(struct server_pool *sp) {
    return sp->n_slowlog == 0 ? 1 : 0;
}
