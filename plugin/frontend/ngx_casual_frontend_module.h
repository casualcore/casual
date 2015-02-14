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

#ifndef _NGX_CASUAL_MODULE_H_
#define _NGX_CASUAL_MODULE_H_

#include <nginx.h>
#include <ngx_core.h>
#include <ngx_http.h>

extern ngx_module_t  ngx_casual_frontend_module;

typedef struct {
    ngx_uint_t                          current;
    ngx_array_t                         *servers;
    ngx_pool_t                          *pool;
    /* keepalive */
    ngx_flag_t                          single;
    ngx_queue_t                         free;
    ngx_queue_t                         cache;
    ngx_uint_t                          active_conns;
    ngx_uint_t                          max_cached;
    ngx_uint_t                          reject;
} ngx_casual_upstream_srv_conf_t;

typedef struct {
    ngx_addr_t                         *addrs;
    ngx_uint_t                          naddrs;
} ngx_casual_upstream_server_t;

typedef struct {
    /* upstream */
    ngx_http_upstream_conf_t            upstream;
} ngx_casual_loc_conf_t;

typedef struct {
    ngx_chain_t                        *response;
    ngx_int_t                           status;
} ngx_casual_ctx_t;


void       *ngx_casual_create_loc_conf(ngx_conf_t *);
char       *ngx_casual_merge_loc_conf(ngx_conf_t *, void *, void *);
char       *ngx_casual_conf_pass(ngx_conf_t *, ngx_command_t *, void *);
void       *ngx_casual_create_upstream_srv_conf(ngx_conf_t *cf);
char       *ngx_casual_conf_server(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

#endif /* _NGX_CASUAL_MODULE_H_ */
