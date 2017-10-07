/*
 * ngx_http_xatmi_common_declaration.h
 *
 *  Created on: 28 aug 2016
 *      Author: 40043017
 */

#ifndef NGX_HTTP_XATMI_COMMON_DECLARATION_H_
#define NGX_HTTP_XATMI_COMMON_DECLARATION_H_


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



#endif /* NGX_HTTP_XATMI_COMMON_DECLARATION_H_ */
