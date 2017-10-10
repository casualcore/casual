/*
 * casual_handler.h
 *
 *  Created on: 9 apr. 2016
 *      Author: hbergk
 */

#ifndef MIDDLEWARE_PLUGIN_CASUAL_HANDLER_H_
#define MIDDLEWARE_PLUGIN_CASUAL_HANDLER_H_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

ngx_int_t casual_call( u_char* service, ngx_str_t call_buffer, ngx_http_request_t* r);
ngx_int_t casual_receive( ngx_int_t descriptor, ngx_str_t* replybuffer, ngx_http_request_t* r);
ngx_str_t errorhandler(ngx_http_request_t* r);


#endif /* MIDDLEWARE_PLUGIN_CASUAL_HANDLER_H_ */
