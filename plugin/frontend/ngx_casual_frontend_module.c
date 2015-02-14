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
#define DDEBUG 1
#endif

#include "ngx_casual_frontend_ddebug.h"
#include "ngx_casual_frontend_handler.h"
#include "ngx_casual_frontend_module.h"
#include "ngx_casual_frontend_util.h"
#include "ngx_casual_frontend_upstream.h"

static ngx_command_t ngx_casual_frontend_module_commands[] = {

   { ngx_string("casual_server"),
     NGX_HTTP_UPS_CONF|NGX_CONF_TAKE1,
     ngx_casual_conf_server,
     NGX_HTTP_SRV_CONF_OFFSET,
     0,
     NULL },

    { ngx_string("casual_pass"),
     NGX_HTTP_LOC_CONF | NGX_HTTP_LIF_CONF | NGX_CONF_TAKE1,
     ngx_casual_conf_pass,
     NGX_HTTP_LOC_CONF_OFFSET,
     0,
     NULL },

   ngx_null_command
};


static ngx_http_module_t ngx_casual_frontend_module_ctx = {
    NULL,                                   /* preconfiguration */
    NULL,                                   /* postconfiguration */

    NULL,                                   /* create main configuration */
    NULL,                                   /* init main configuration */

    ngx_casual_create_upstream_srv_conf,    /* create server configuration */
    NULL,                                   /* merge server configuration */

    ngx_casual_create_loc_conf,           /* create location configuration */
    ngx_casual_merge_loc_conf             /* merge location configuration */
};

ngx_module_t ngx_casual_frontend_module = {
    NGX_MODULE_V1,
    &ngx_casual_frontend_module_ctx,      /* module context */
    ngx_casual_frontend_module_commands,  /* module directives */
    NGX_HTTP_MODULE,               /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    NULL,                          /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};

void *
ngx_casual_create_upstream_srv_conf(ngx_conf_t *cf)
{
    ngx_casual_upstream_srv_conf_t  *conf;
    ngx_pool_cleanup_t                *cln;

    DD("entering");

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_casual_upstream_srv_conf_t));
    if (conf == NULL) {
        DD("returning NULL");
        return NULL;
    }

    /*
     * set by ngx_pcalloc():
     *
     *     conf->peers = NULL
     *     conf->current = 0
     *     conf->servers = NULL
     *     conf->free = { NULL, NULL }
     *     conf->cache = { NULL, NULL }
     *     conf->active_conns = 0
     *     conf->reject = 0
     */

    conf->pool = cf->pool;

    /* enable keepalive (single) by default */
    conf->max_cached = 10;
    conf->single = 1;

    cln = ngx_pool_cleanup_add(cf->pool, 0);
    cln->data = conf;

    DD("returning");
    return conf;
}

void *
ngx_casual_create_loc_conf(ngx_conf_t *cf)
{
    ngx_casual_loc_conf_t*  conf;

    DD("entering");

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_casual_loc_conf_t));
    if (conf == NULL) {
        DD("returning NULL");
        return NULL;
    }

    /*
     * set by ngx_pcalloc():
     *
     *     conf->upstream.* = 0 / NULL
     *     conf->upstream_cv = NULL
     *     conf->query.methods_set = 0
     *     conf->query.methods = NULL
     *     conf->query.def = NULL
     *     conf->output_binary = 0
     */

    conf->upstream.connect_timeout = NGX_CONF_UNSET_MSEC;
    conf->upstream.read_timeout = NGX_CONF_UNSET_MSEC;

    /* the hardcoded values */
    conf->upstream.cyclic_temp_file = 0;
    conf->upstream.buffering = 1;
    conf->upstream.ignore_client_abort = 1;
    conf->upstream.send_lowat = 0;
    conf->upstream.bufs.num = 0;
    conf->upstream.busy_buffers_size = 0;
    conf->upstream.max_temp_file_size = 0;
    conf->upstream.temp_file_write_size = 0;
    conf->upstream.intercept_errors = 1;
    conf->upstream.intercept_404 = 1;
    conf->upstream.pass_request_headers = 0;
    conf->upstream.pass_request_body = 0;

   DD("returning");
    return conf;
}

char *
ngx_casual_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_casual_loc_conf_t  *prev = parent;
    ngx_casual_loc_conf_t  *conf = child;

    DD("entering");

    ngx_conf_merge_msec_value(conf->upstream.connect_timeout,
                              prev->upstream.connect_timeout, 10000);

    ngx_conf_merge_msec_value(conf->upstream.read_timeout,
                              prev->upstream.read_timeout, 30000);


    if (conf->upstream.upstream == NULL) {
        conf->upstream.upstream = prev->upstream.upstream;
    }

    DD("returning NGX_CONF_OK");
    return NGX_CONF_OK;
}

char *
ngx_casual_conf_pass(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
   ngx_str_t                         *value = cf->args->elts;
   ngx_casual_loc_conf_t             *pglcf = conf;
   ngx_http_core_loc_conf_t          *clcf;
   ngx_url_t                          url;

   DD("entering");

   if ((pglcf->upstream.upstream != NULL) ) {
       DD("returning");
       return "is duplicate";
   }

   if (value[1].len == 0) {
       ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                          "postgres: empty upstream in \"%V\" directive",
                          &cmd->name);

       DD("returning NGX_CONF_ERROR");
       return NGX_CONF_ERROR;
   }

   clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

   clcf->handler = ngx_casual_handler;

   if (clcf->name.data[clcf->name.len - 1] == '/') {
       clcf->auto_redirect = 1;
   }

    /* simple value */
    DD("simple value");

    ngx_memzero(&url, sizeof(ngx_url_t));

    url.url = value[1];
    url.no_resolve = 1;
    ngx_str_set(&url.host, "localhost");
    url.port = 4321;
    url.default_port = 4321;

    pglcf->upstream.upstream = ngx_http_upstream_add(cf, &url, 0);
    if (pglcf->upstream.upstream == NULL) {
        DD("returning NGX_CONF_ERROR");
        return NGX_CONF_ERROR;
    }

    DD("returning NGX_CONF_OK");
    return NGX_CONF_OK;
}

/*
 * Based on: ngx_http_upstream.c/ngx_http_upstream_server
 * Copyright (C) Igor Sysoev
 */
char *
ngx_casual_conf_server(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_str_t                         *value = cf->args->elts;
    ngx_casual_upstream_srv_conf_t  *pgscf = conf;
    ngx_casual_upstream_server_t    *pgs;
    ngx_http_upstream_srv_conf_t      *uscf;
    ngx_url_t                          u;

    DD("entering");

    uscf = ngx_http_conf_get_module_srv_conf(cf, ngx_http_upstream_module);

    if (pgscf->servers == NULL) {
        pgscf->servers = ngx_array_create(cf->pool, 4,
                             sizeof(ngx_casual_upstream_server_t));
        if (pgscf->servers == NULL) {
            DD("returning NGX_CONF_ERROR");
            return NGX_CONF_ERROR;
        }

        uscf->servers = pgscf->servers;
    }

    pgs = ngx_array_push(pgscf->servers);
    if (pgs == NULL) {
        DD("returning NGX_CONF_ERROR");
        return NGX_CONF_ERROR;
    }

    ngx_memzero(pgs, sizeof(ngx_casual_upstream_server_t));

    /* parse the first name:port argument */

    ngx_memzero(&u, sizeof(ngx_url_t));

    u.url = value[1];
    u.default_port = 4321; /* Not used */

    if (ngx_parse_url(cf->pool, &u) != NGX_OK) {
        if (u.err) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "casual: %s in upstream \"%V\"",
                               u.err, &u.url);
        }

        DD("returning NGX_CONF_ERROR");
        return NGX_CONF_ERROR;
    }

    pgs->addrs = u.addrs;
    pgs->naddrs = u.naddrs;

    //uscf->peer.init_upstream = ngx_casual_upstream_init;

    DD("returning NGX_CONF_OK");
    return NGX_CONF_OK;
}

