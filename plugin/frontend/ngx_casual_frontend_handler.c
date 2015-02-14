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
#include "ngx_casual_frontend_handler.h"
#include "ngx_casual_frontend_module.h"
#include "ngx_casual_frontend_processor.h"
#include "ngx_casual_frontend_util.h"


ngx_int_t
ngx_casual_handler(ngx_http_request_t *r)
{
    ngx_casual_loc_conf_t   *casual_local_conf;
    ngx_casual_ctx_t        *pgctx;
    ngx_http_core_loc_conf_t  *clcf;
    ngx_http_upstream_t       *u;
    ngx_connection_t          *c;
    ngx_str_t                  host;
    ngx_url_t                  url;
    ngx_int_t                  rc;

    ngx_http_upstream_srv_conf_t* upstream_conf;

    DD("entering");

    if (r->subrequest_in_memory) {
        /* TODO: add support for subrequest in memory by
         * emitting output into u->buffer instead */

        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "casual: ngx_casual module does not support"
                      " subrequests in memory");

        DD("returning NGX_HTTP_INTERNAL_SERVER_ERROR");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    DD("1");
    casual_local_conf = ngx_http_get_module_loc_conf(r, ngx_casual_frontend_module);

    if (ngx_http_upstream_create(r) != NGX_OK) {
        DD("returning NGX_HTTP_INTERNAL_SERVER_ERROR");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    u = r->upstream;

    pgctx = ngx_pcalloc(r->pool, sizeof(ngx_casual_ctx_t));
    if (pgctx == NULL) {
        DD("returning NGX_HTTP_INTERNAL_SERVER_ERROR");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    /*
     * set by ngx_pcalloc():
     *
     *     pgctx->response = NULL
     *     pgctx->var_query = { 0, NULL }
     *     pgctx->variables = NULL
     *     pgctx->status = 0
     */

    DD("2");
    ngx_http_set_ctx(r, pgctx, ngx_casual_frontend_module);

    u->schema.len = sizeof("http://") - 1;
    u->schema.data = (u_char *) "http://";

    u->output.tag = (ngx_buf_tag_t) &ngx_casual_frontend_module;

    u->conf = &casual_local_conf->upstream;

    u->create_request = ngx_casual_create_request;
    u->reinit_request = ngx_casual_reinit_request;
    u->process_header = ngx_casual_process_header;
    u->abort_request = ngx_casual_abort_request;
    u->finalize_request = ngx_casual_finalize_request;

    /* we bypass the upstream input filter mechanism in
     * ngx_http_upstream_process_headers */

    u->input_filter_init = ngx_casual_input_filter_init;
    u->input_filter = ngx_casual_input_filter;
    u->input_filter_ctx = NULL;

    r->main->count++;

    DD("3");

    /* override the read/write event handler to our own */
    u->write_event_handler = ngx_casual_write_event_handler;
    u->read_event_handler = ngx_casual_read_event_handler;

    ngx_memzero(&url, sizeof(ngx_url_t));

    u_int port = 4321;

    url.url.len = sizeof("127.0.0.1:4321") - 1;
    url.url.data = (u_char*)"127.0.0.1:4321";
    url.default_port = port;
    url.uri_part = 1;
    url.no_resolve = 1;

    if (ngx_parse_url(r->pool, &url) != NGX_OK) {
        if (url.err) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "%s in upstream \"%V\"", url.err, &url.url);
        }

        return NGX_ERROR;
    }


    u->resolved = ngx_pcalloc(r->pool, sizeof(ngx_http_upstream_resolved_t));
    if (u->resolved == NULL) {
        return NGX_ERROR;
    }

    if (url.addrs && url.addrs[0].sockaddr) {
        u->resolved->sockaddr = url.addrs[0].sockaddr;
        u->resolved->socklen = url.addrs[0].socklen;
        u->resolved->naddrs = 1;
        u->resolved->host = url.addrs[0].name;

    } else {
        u->resolved->host = url.host;
        u->resolved->port = (in_port_t) (url.no_port ? port : url.port);
        u->resolved->no_port = url.no_port;
    }


    DD("4");
    rc = ngx_http_read_client_request_body(r, ngx_http_upstream_init);

    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
            return rc;
    }
    DD("5");
    DD("returning NGX_DONE");
    return NGX_DONE;
}

void
ngx_casual_write_event_handler(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ngx_connection_t  *casual_connection;

    DD("entering");


    /* just to ensure u->reinit_request always gets called for
     * upstream_next */
    u->request_sent = 1;

    casual_connection = u->peer.connection;

    if (casual_connection->write->timedout) {
        DD("casual connection write timeout");

        ngx_casual_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_TIMEOUT);

        DD("returning");
        return;
    }

    ngx_casual_process_events(r);

    DD("returning");
}

void
ngx_casual_read_event_handler(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ngx_connection_t  *casual_connection;

    DD("entering");


    /* just to ensure u->reinit_request always gets called for
     * upstream_next */
    u->request_sent = 1;

    ngx_casual_process_events(r);

    DD("returning");
}

ngx_int_t
ngx_casual_create_request(ngx_http_request_t *r)
{
    DD("entering");

    r->upstream->request_bufs = NULL;

    DD("returning NGX_OK");
    return NGX_OK;
}

ngx_int_t
ngx_casual_reinit_request(ngx_http_request_t *r)
{
    ngx_http_upstream_t  *u;

    DD("entering");

    u = r->upstream;

    /* override the read/write event handler to our own */
    u->write_event_handler = ngx_casual_write_event_handler;
    u->read_event_handler = ngx_casual_read_event_handler;

    DD("returning NGX_OK");
    return NGX_OK;
}

void
ngx_casual_abort_request(ngx_http_request_t *r)
{
    DD("entering & returning (dummy function)");
}

void
ngx_casual_finalize_request(ngx_http_request_t *r, ngx_int_t rc)
{
    ngx_casual_ctx_t  *pgctx;

    DD("entering");

    if (rc == NGX_OK) {
        pgctx = ngx_http_get_module_ctx(r, ngx_casual_frontend_module);

        ngx_casual_output_chain(r, pgctx->response);
    }

    DD("returning");
}

ngx_int_t
ngx_casual_process_header(ngx_http_request_t *r)
{
    DD("entering");

    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                  "casual: ngx_casual_process_header should not"
                  " be called by the upstream");

    DD("returning NGX_ERROR");
    return NGX_ERROR;
}

ngx_int_t
ngx_casual_input_filter_init(void *data)
{
    ngx_http_request_t  *r = data;

    DD("entering");

    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                  "casual: ngx_casual_input_filter_init should not"
                  " be called by the upstream");

    DD("returning NGX_ERROR");
    return NGX_ERROR;
}

ngx_int_t
ngx_casual_input_filter(void *data, ssize_t bytes)
{
    ngx_http_request_t  *r = data;

    DD("entering");

    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                  "casual: ngx_casual_input_filter should not"
                  " be called by the upstream");

    DD("returning NGX_ERROR");
    return NGX_ERROR;
}
