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
#include "ngx_casual_frontend_processor.h"
#include "ngx_casual_frontend_util.h"

void
ngx_casual_process_events(ngx_http_request_t *r)
{
    ngx_http_upstream_t*         u = r->upstream;
    ngx_connection_t*            casual_connection = u->peer.connection;
    ngx_casual_upstream_peer_data_t*  casual_data = u->peer.data;
    ngx_int_t                    rc;
    char text[80];

    DD("entering");

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, casual_connection->log, 0,
                   "casual: process events");

    switch (casual_data->state) {
    case state_casual_tpacall:
        DD("state_casual_tpacall");
        rc = ngx_casual_upstream_send_query(r, casual_connection, casual_data);
        break;
    case state_casual_tpgetreply:
        DD("state_casual_tpgetreply");
        rc = ngx_casual_upstream_get_result(r, casual_connection, casual_data);
        break;
    default:
        sprintf(text,"unknown state:%d", casual_data->state);
        DD(text);
        ngx_log_error(NGX_LOG_ERR, casual_connection->log, 0,
                      "casual: unknown state:%d", casual_data->state);

        goto failed;
    }

    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
        ngx_casual_upstream_finalize_request(r, u, rc);
    } else if (rc == NGX_ERROR) {
        goto failed;
    }

    DD("returning");
    return;

failed:
    ngx_casual_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_ERROR);

    DD("returning");
}


ngx_int_t
ngx_casual_upstream_send_query(ngx_http_request_t *r, ngx_connection_t *casual_connection,
      ngx_casual_upstream_peer_data_t *casual_data)
{
    ngx_casual_loc_conf_t  *casual_local_config;
    ngx_int_t                 casual_result;

    DD("entering");

    casual_local_config = ngx_http_get_module_loc_conf(r, ngx_casual_frontend_module);

    u_char *body = NULL;

    long content_length = r->headers_in.content_length_n;
     //
     // Any data from caller?
     //
     if (content_length > 0)
     {
        ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, (char*) "Copying body.");

        body = ngx_palloc(r->pool, content_length);
        ngx_memcpy(body, r->request_body->bufs->buf->start, content_length);
        ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, (char*) body);
     }
     else
     {
        content_length = 0;
        ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, (char*) "Empty body.");

        body = ngx_palloc(r->pool, content_length);
        ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, (char*) body);
     }

     DD("6");
    /* set result timeout */
    ngx_add_timer(casual_connection->read, r->upstream->conf->read_timeout);

    DD("query sent successfully");
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, casual_connection->log, 0,
                   "casual: query sent successfully");

    casual_connection->log->action = "waiting for result from tpacall";
    casual_data->state = state_casual_tpgetreply;

    DD("returning NGX_DONE");
    return NGX_DONE;
}

ngx_int_t
ngx_casual_upstream_get_result(ngx_http_request_t *r, ngx_connection_t *casual_connection,
      ngx_casual_upstream_peer_data_t *casual_data)
{
    ngx_int_t        rc;

    DD("entering");

    /* remove connection timeout from re-used keepalive connection */
    if (casual_connection->write->timer_set) {
        ngx_del_timer(casual_connection->write);
    }


    // make call

    u_char* res;

    casual_connection->log->action = "processing result from casual";
    rc = ngx_casual_process_response(r, res);


    DD("result processed successfully");

    casual_data->state = state_casual_tpacall;

    DD("returning");
    return ngx_casual_upstream_done(r, r->upstream, casual_data);
}

ngx_int_t
ngx_casual_upstream_done(ngx_http_request_t *r, ngx_http_upstream_t *u,
    ngx_casual_upstream_peer_data_t *casual_data)
{
    ngx_casual_ctx_t  *casual_ctx;

    DD("entering");

    /* flag for keepalive */
    u->headers_in.status_n = NGX_HTTP_OK;

    casual_ctx = ngx_http_get_module_ctx(r, ngx_casual_frontend_module);

    if (casual_ctx->status >= NGX_HTTP_SPECIAL_RESPONSE) {
        ngx_casual_upstream_finalize_request(r, u, casual_ctx->status);
    } else {
        ngx_casual_upstream_finalize_request(r, u, NGX_OK);
    }

    DD("returning NGX_DONE");
    return NGX_DONE;
}

ngx_int_t
ngx_casual_process_response(ngx_http_request_t *r, u_char *res)
{
    ngx_casual_ctx_t*         casual_context;
    ngx_chain_t*              buffer_chain;
    ngx_buf_t*                buffer;
    size_t                    size = strlen( res);

    DD("entering");

    casual_context = ngx_http_get_module_ctx(r, ngx_casual_frontend_module);

    buffer = ngx_create_temp_buf(r->pool, size);
    if (buffer == NULL) {
        DD("returning NGX_ERROR");
        return NGX_ERROR;
    }

    buffer_chain = ngx_alloc_chain_link(r->pool);
    if (buffer_chain == NULL) {
        DD("returning NGX_ERROR");
        return NGX_ERROR;
    }

    buffer_chain->buf = buffer;
    buffer->memory = 1;
    buffer->tag = r->upstream->output.tag;

    buffer->last = ngx_copy(buffer->last, res, size);

    if (buffer->last != buffer->end) {
        DD("returning NGX_ERROR");
        return NGX_ERROR;
    }

    buffer_chain->next = NULL;

    /* set output response */
    casual_context->response = buffer_chain;

    DD("returning NGX_DONE");
    return NGX_DONE;
}

ngx_int_t
ngx_casual_output_chain(ngx_http_request_t *r, ngx_chain_t *cl)
{
    ngx_http_upstream_t       *u = r->upstream;
    ngx_http_core_loc_conf_t  *clcf;
    ngx_casual_loc_conf_t   *pglcf;
    ngx_casual_ctx_t        *pgctx;
    ngx_int_t                  rc;
    char text[80];

    DD("entering");

    if (!r->header_sent) {
        ngx_http_clear_content_length(r);

        pglcf = ngx_http_get_module_loc_conf(r, ngx_casual_frontend_module);
        pgctx = ngx_http_get_module_ctx(r, ngx_casual_frontend_module);

        r->headers_out.status = pgctx->status ? abs(pgctx->status)
                                              : NGX_HTTP_OK;

        /* default type for output value|row */
        clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

        r->headers_out.content_type = clcf->default_type;
        r->headers_out.content_type_len = clcf->default_type.len;

        r->headers_out.content_type_lowcase = NULL;

        rc = ngx_http_send_header(r);
        if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
            sprintf(text,"returning rc:%d", (int) rc);
            DD(text);
            return rc;
        }
    }

    if (cl == NULL) {
        DD("returning NGX_DONE");
        return NGX_DONE;
    }

    rc = ngx_http_output_filter(r, cl);
    if (rc == NGX_ERROR || rc > NGX_OK) {
        sprintf(text,"returning rc:%d", (int) rc);
        DD(text);
        return rc;
    }

    ngx_chain_update_chains(r->pool, &u->free_bufs, &u->busy_bufs, &cl,
                            u->output.tag);

    sprintf(text,"returning rc:%d", (int) rc);
    DD(text);

    return rc;
}
