/*
 * casual_handler.c
 *
 *  Created on: 9 apr. 2016
 *      Author: hbergk
 */
#include "casual_handler.h"

//
// Casual
//
#include "xatmi.h"

ngx_int_t casual_call( u_char* service, ngx_str_t call_buffer, ngx_http_request_t* r)
{
	char* buffer = tpalloc( CASUAL_BUFFER_JSON_TYPE, CASUAL_BUFFER_JSON_SUBTYPE, call_buffer.len);
	int calling_descriptor = -1;

	if (buffer)
	{
	  ngx_log_debug0(NGX_LOG_DEBUG_ALL, r->connection->log, 0,  "casual: Preparing for tpacall");
	  strncpy(buffer, (const char*)call_buffer.data, call_buffer.len);

	  calling_descriptor = tpacall( (const char *)service, buffer, call_buffer.len, 0);
	}

	tpfree( buffer);

	return calling_descriptor;
}

ngx_int_t casual_receive( ngx_int_t descriptor, ngx_str_t* replybuffer, ngx_http_request_t* r)
{
   char* buffer = tpalloc( CASUAL_BUFFER_JSON_TYPE, CASUAL_BUFFER_JSON_SUBTYPE, 1024);
   if (buffer)
   {
	  int reply = tpgetrply((int*)&descriptor, &buffer, (long*)&replybuffer->len, TPNOBLOCK);
	  if ( reply == -1 )
	  {
		 tpfree( buffer);
		 if (tperrno == TPEBLOCK)
		 {
			return NGX_AGAIN;
		 }
		 else
		 {
			return NGX_ERROR;
		 }
	  }

	  replybuffer->data = ngx_pcalloc( r->pool, replybuffer->len);
	  ngx_memcpy( replybuffer->data, buffer, replybuffer->len);
	  tpfree( buffer);
   }
   return NGX_OK;
}

ngx_str_t errorhandler(ngx_http_request_t* r)
{
   ngx_str_t error;
   u_char message[1024];
   ngx_sprintf(message, "{'error' : '%s'}", tperrnostring(tperrno));
   error.len = ngx_strlen(message) + 1;
   error.data = ngx_pcalloc(r->pool, error.len);
   ngx_cpystrn(error.data, message, error.len);
   ngx_log_debug1(NGX_LOG_ERR, r->connection->log, 0, "casual: error: %V", &error);
   r->headers_out.status = NGX_HTTP_BAD_REQUEST;
   return error;
}
