/*
 * Copyright (c) 2010, FRiCKLE Piotr Sikora <info@frickle.com>
 * Copyright (c) 2009-2010, Xiaozhe Wang <chaoslawful@gmail.com>
 * Copyright (c) 2009-2010, Yichun Zhang <agentzh@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DDEBUG
#define DDEBUG 0
#endif

#include "ngx_casual_frontend_ddebug.h"
#include "ngx_casual_frontend_module.h"
#include "ngx_casual_frontend_upstream.h"
#include "ngx_casual_frontend_processor.h"


ngx_int_t
ngx_casual_upstream_init(ngx_conf_t *cf, ngx_http_upstream_srv_conf_t *uscf)
{
    ngx_casual_upstream_srv_conf_t  *pgscf;
    ngx_casual_upstream_server_t    *server;
    ngx_uint_t                         i, n;

    DD("entering");

    //uscf->peer.init = ngx_casual_upstream_init_peer;

    pgscf = ngx_http_conf_upstream_srv_conf(uscf, ngx_casual_frontend_module);



    /* pgscf->servers != NULL */

    server = uscf->servers->elts;

    n = 0;

    for (i = 0; i < uscf->servers->nelts; i++) {
        n += server[i].naddrs;
    }

    n = 0;

    pgscf->active_conns = 0;

    DD("returning NGX_OK");
    return NGX_OK;
}

ngx_int_t
ngx_casual_upstream_init_peer(ngx_http_request_t *r,
    ngx_http_upstream_srv_conf_t *uscf)
{
    ngx_casual_upstream_peer_data_t  *pgdt;
    ngx_casual_upstream_srv_conf_t   *pgscf;
    ngx_casual_loc_conf_t            *pglcf;
    ngx_casual_ctx_t                 *pgctx;
    ngx_http_upstream_t                *u;

    DD("entering");

    pgdt = ngx_pcalloc(r->pool, sizeof(ngx_casual_upstream_peer_data_t));
    if (pgdt == NULL) {
        goto failed;
    }

    u = r->upstream;

    pgdt->upstream = u;
    pgdt->request = r;

    pgscf = ngx_http_conf_upstream_srv_conf(uscf, ngx_casual_frontend_module);
    pglcf = ngx_http_get_module_loc_conf(r, ngx_casual_frontend_module);
    pgctx = ngx_http_get_module_ctx(r, ngx_casual_frontend_module);

    pgdt->srv_conf = pgscf;
    pgdt->loc_conf = pglcf;

    u->peer.data = pgdt;
    u->peer.get = ngx_casual_upstream_get_peer;
    u->peer.free = ngx_casual_upstream_free_peer;


    DD("using simple value");


    DD("returning NGX_OK");
    return NGX_OK;

failed:
    DD("returning NGX_ERROR");
    return NGX_ERROR;
}
ngx_int_t
ngx_casual_upstream_get_peer(ngx_peer_connection_t *pc, void *data)
{
    ngx_casual_upstream_peer_data_t  *pgdt = data;
    ngx_connection_t                 *pgxc = NULL;
    ngx_event_t                        *rev, *wev;
    ngx_int_t                           rc;

    DD("entering");

    pgdt->failed = 0;

    pgxc = pc->connection = ngx_get_connection(0, pc->log);

    if (pgxc == NULL) {
        ngx_log_error(NGX_LOG_ERR, pc->log, 0,
                      "casual: failed to get a free nginx connection");

        goto invalid;
    }

    peer = &peers->peer[pgscf->current++];

   pgdt->name.len = peer->name.len;
   pgdt->name.data = peer->name.data;

   pgdt->sockaddr = *peer->sockaddr;

   pc->name = &pgdt->name;
   pc->sockaddr = &pgdt->sockaddr;
   pc->socklen = peer->socklen;
   pc->cached = 0;


    pgxc->log = pc->log;
    pgxc->log_error = pc->log_error;
    pgxc->number = ngx_atomic_fetch_add(ngx_connection_counter, 1);

    rev = pgxc->read;
    wev = pgxc->write;

    rev->log = pc->log;
    wev->log = pc->log;

    /* register the connection with postgres connection fd into the
     * nginx event model */

    if (ngx_event_flags & NGX_USE_RTSIG_EVENT) {
        DD("NGX_USE_RTSIG_EVENT");
        if (ngx_add_conn(pgxc) != NGX_OK) {
            goto bad_add;
        }

    } else if (ngx_event_flags & NGX_USE_CLEAR_EVENT) {
        DD("NGX_USE_CLEAR_EVENT");
        if (ngx_add_event(rev, NGX_READ_EVENT, NGX_CLEAR_EVENT) != NGX_OK) {
            goto bad_add;
        }

        if (ngx_add_event(wev, NGX_WRITE_EVENT, NGX_CLEAR_EVENT) != NGX_OK) {
            goto bad_add;
        }

    } else {
        DD("NGX_USE_LEVEL_EVENT");
        if (ngx_add_event(rev, NGX_READ_EVENT, NGX_LEVEL_EVENT) != NGX_OK) {
            goto bad_add;
        }

        if (ngx_add_event(wev, NGX_WRITE_EVENT, NGX_LEVEL_EVENT) != NGX_OK) {
            goto bad_add;
        }
    }

    pgxc->log->action = "connecting to casual server";
    pgdt->state = state_casual_tpacall;

    ngx_casual_process_events(pgdt->request);

    DD("returning NGX_AGAIN");
    return NGX_AGAIN;

bad_add:
        ngx_log_error(NGX_LOG_ERR, pc->log, 0,
                      "casual: failed to add nginx connection");

invalid:

  DD("returning NGX_ERROR");
  return NGX_ERROR;
}

void
ngx_casual_upstream_free_peer(ngx_peer_connection_t *pc,
    void *data, ngx_uint_t state)
{
    DD("entering - ngx_casual_upstream_free_peer");

    DD("returning - ngx_casual_upstream_free_peer");
}

ngx_flag_t
ngx_casual_upstream_is_my_peer(const ngx_peer_connection_t *peer)
{
    DD("entering & returning");
    return (peer->get == ngx_casual_upstream_get_peer);
}
