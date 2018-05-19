/** 
 ** Copyright (c) 2015, The casual project
 **
 ** This software is licensed under the MIT license, https://opensource.org/licenses/MIT
 **/

#pragma once
//
// Unique configuration per request
//
typedef struct {
   u_char* service;
   u_char* protocol;
   ngx_int_t calling_descriptor;
   ngx_int_t numberOfCalls;
   ngx_msec_t timeout;
   ngx_str_t call_buffer;
   ngx_str_t reply_buffer;
   ngx_int_t state;
   unsigned waiting_more_body:1;
} ngx_http_xatmi_ctx_t;


