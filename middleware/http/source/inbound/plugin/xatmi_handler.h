/*
 * tuxedo_handler.h
 *
 *  Created on: 9 apr. 2016
 *      Author: hbergk
 */

#ifndef MIDDLEWARE_PLUGIN_TUXEDO_HANDLER_H_
#define MIDDLEWARE_PLUGIN_TUXEDO_HANDLER_H_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "ngx_http_xatmi_common_declaration.h"
#include "http/inbound/caller.h"

ngx_int_t xatmi_call( ngx_http_xatmi_ctx_t* client_context, ngx_http_request_t* r);
ngx_int_t xatmi_receive( ngx_http_xatmi_ctx_t* client_context, ngx_http_request_t* r);
void xatmi_cancel( ngx_http_xatmi_ctx_t* client_context, ngx_http_request_t* r);
void xatmi_terminate( );
ngx_int_t xatmi_max_service_length();

void errorreporter(ngx_http_request_t* r, ngx_http_xatmi_ctx_t* client_context, ngx_int_t status, const char* message);
void errorhandler(ngx_http_request_t* r, ngx_http_xatmi_ctx_t* client_context);

ngx_int_t buffer_size( ngx_int_t value);
ngx_int_t get_header_length(ngx_http_request_t *r);
ngx_int_t copy_headers( ngx_http_request_t *r, CasualHeader* headers);
#endif /* MIDDLEWARE_PLUGIN_TUXEDO_HANDLER_H_ */
