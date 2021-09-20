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

#include <nc_monitor.h>
#include <nc_rbtree.h>

struct rbtree monitor_tree;
struct rbnode monitor_sentinel_node;

void monitor_init()
{
    rbtree_init(&monitor_tree, &monitor_sentinel_node);
}

void monitor_deinit(struct context *ctx)
{
    struct rbnode *node = NULL;

    while ((node = rbtree_min(&monitor_tree) != NULL))
    {
        core_close(ctx, node->data);
        rbtree_delete(&monitor_tree, node);
    }
}

int mointor_is_empty()
{
    return rbtree_is_empty(&monitor_tree);
}

rstatus_t add_to_monitor(struct conn *c)
{
    ASSERT(c->client);

    struct rbnode *node = nc_alloc(sizeof(struct rbnode));
    if (node == NULL)
    {
        return NC_ENOMEM;
    }

    c->monitor_client = 1;
    node->key = c->sd;
    node->data = c;

    rbtree_insert(&monitor_tree, node);
}

void del_from_monitor(struct conn *c)
{
    ASSERT(c->monitor_client);

    struct rbnode *node = rbtree_find(&monitor_tree, c->sd);
    ASSERT(node != NULL);
    rbtree_delete(&monitor_tree, node);
}

struct monitor_data
{
    struct msg *m;
    struct conn *c;
    struct context *ctx;
    struct string *d;
};

static void monitor_callback(struct rbnode *node, void *data)
{
    struct monitor_data *mdata = data;
    struct conn *req_c = node->data;

    struct msg *req = req_get(req_c);
    struct msg *rsp = msg_get(req_c, 0, mdata->c->redis);

    req->peer = rsp;
    rsp->peer = req;

    req->done = 1;
    rsp->done = 1;

    msg_append_full(rsp, mdata->d->data, mdata->d->len);
    req_c->enqueue_outq(mdata->ctx, req_c, req);
    event_add_out(mdata->ctx->evb, req_c);
}

rstatus_t make_monitor(struct context *ctx, struct conn *c, struct msg *m)
{
    ASSERT(c->client);

    struct string monitor_message = null_string;
    struct monitor_data mdata = {0};
    mdata.m = m;
    mdata.c = c;
    mdata.d = &monitor_message;
    mdata.ctx = ctx;
    struct keypos *kpos = array_get(m->keys, 0);

    string_printf(&monitor_message, "+%ld.%06ld [%s] command=%s key0=",
                    m->start_ts/1000000, m->start_ts%1000000, 
                    nc_unresolve_peer_desc(c->sd),
                    (msg_type_string(m->type))->data);
    string_cat_len(&monitor_message, kpos->start, kpos->end - kpos->start);
    string_cat_len(&monitor_message, "\r\n", 2);

    rbtree_inorder_traversal(monitor_tree.root, monitor_tree.sentinel, monitor_callback, &mdata);

    string_deinit(&monitor_message);
    return NC_OK;
}
