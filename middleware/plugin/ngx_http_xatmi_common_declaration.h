/** 
 ** Copyright (c) 2015, The casual project
 **
 ** This software is licensed under the MIT license, https://opensource.org/licenses/MIT
 **/
#pragma once

#include <sys/types.h>
#include <ngx_config.h>
#include <http/inbound/caller.h>

//
// Unique configuration per request
//
typedef struct {
   u_char* service;
   u_char* protocol;
   u_char lookup_uuid[ UUID_SIZE + 1];
   ngx_int_t descriptor;
   ngx_int_t code;
   ngx_int_t number_of_calls;
   ngx_msec_t timeout;
   ngx_str_t call;
   ngx_str_t reply;
   ngx_int_t state;
   unsigned waiting_more_body:1;
} ngx_http_xatmi_ctx_t;


