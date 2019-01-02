/** 
 **  Copyright (c) 2015, The casual project
 **
 ** This software is licensed under the MIT license, https://opensource.org/licenses/MIT
 */

#include "xatmi_handler.h"

// Global representation av senaste xatmi errorkod
long xatmi_tperrno;

//Global representation av senaste context där fel uppstod.
enum xatmi_context xatmi_error_context;

buffer_type copy( ngx_str_t source)
{
   buffer_type buffer;
   buffer.data = (char*)malloc( source.len + 1);
   memcpy( buffer.data, (char*)source.data, source.len);
   buffer.data[source.len] = '\0';
   buffer.size = source.len;
   return buffer;
}

ngx_str_t copy_buffer( buffer_type source, ngx_http_request_t* r)
{
   ngx_str_t destination;
   destination.data = ngx_pcalloc( r->pool, source.size + 1);
   ngx_memcpy( destination.data, source.data, source.size);
   destination.len = source.size;
   destination.data[source.size] = '\0';
   return destination;
}

ngx_str_t copy_data( const char* const source, ngx_http_request_t* r)
{
   ngx_str_t destination;
   ngx_int_t size = strlen( source);
   destination.data = ngx_pcalloc( r->pool,  size + 1);
   ngx_memcpy( destination.data, source, size);
   destination.len = size;
   destination.data[size] = '\0';
   return destination;
}

ngx_int_t set_custom_header_in_headers_out(ngx_http_request_t* r, header_type header_out)
{
   for ( int i = 0; i < header_out.size; i++)
   {
      ngx_table_elt_t* h;

      /*
      All we have to do is just to allocate the header...
      */
      h = ngx_list_push(&r->headers_out.headers);
      if (h == NULL) {
         return NGX_ERROR;
      }

      /*
      ... setup the header key ...
      */
      h->key = copy_data( header_out.data[i].key, r);
      ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: key [%V]", &h->key);


      /*
      ... and the value.
      */
      h->value = copy_data( header_out.data[i].value, r);
      ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: value [%V]", &h->value);


      /*
      Mark the header as not deleted.
      */
      h->hash = 1;
   }
    return NGX_OK;
}

ngx_int_t xatmi_call( ngx_http_xatmi_ctx_t* client_context, ngx_http_request_t* r)
{
   ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: calling service [%s]", client_context->service);
   ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: call [%V]", &client_context->call);

   casual_buffer_type transport;
   strcpy( transport.service, (char*)client_context->service);
   strcpy( transport.protocol, (char*)client_context->protocol);

   transport.lookup_key = client_context->lookup_key;

   ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: lookup_key [%d]", client_context->lookup_key);

   long response = casual_xatmi_lookup( &transport);

   if (response == AGAIN)
   {
      client_context->lookup_key = transport.lookup_key;
      ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: lookup_key [%d]", client_context->lookup_key);
      return NGX_AGAIN;
   }
   else if ( response == ERROR)
   {
      xatmi_error_context = transport.context;
      xatmi_tperrno = transport.code;
      client_context->reply = copy_buffer( transport.payload,  r);
      client_context->code = transport.code;
      set_custom_header_in_headers_out( r, transport.header_out);
      free( transport.payload.data);
      return NGX_ERROR;
   }

   //
   // Service manager ready, time to make the call
   //
   ngx_int_t headersize = get_header_length(r);
   ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: headerlength [%d]", headersize);
   header_data_type* header = NULL;
   if (headersize > 0)
   {
      header = (header_data_type*)malloc( headersize * sizeof(header_data_type));

      copy_headers(r, header);
   }

   transport.header_in.data = header;
   transport.header_in.size = headersize;
   transport.header_out.data = 0;
   transport.header_out.size = 0;
   transport.payload = copy( client_context->call);
   transport.parameter = copy( r->args);

   //
   // Make the call
   //
   response = casual_xatmi_send( &transport);

   if (response == ERROR)
   {
     xatmi_error_context = transport.context;
     xatmi_tperrno = transport.code;
     client_context->reply = copy_buffer( transport.payload,  r);
     client_context->code = transport.code;
     free( transport.header_in.data);
     free( transport.payload.data);
     free( transport.parameter.data);
     return NGX_ERROR;
   }

   client_context->descriptor = transport.descriptor;

   free( transport.header_in.data);
   free( transport.payload.data);
   free( transport.parameter.data);
   ngx_log_debug0(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: calling service - end");

   return transport.descriptor;
}

ngx_int_t xatmi_receive( ngx_http_xatmi_ctx_t* client_context, ngx_http_request_t* r)
{
   ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0,  "xatmi: receiving... descriptor [%d]", client_context->descriptor);

   casual_buffer_type transport;
   transport.header_in.size = 0; // No header needed
   transport.descriptor = client_context->descriptor;
   transport.header_out.data = 0;
   transport.header_out.size = 0;

   strcpy( transport.protocol, (char*)client_context->protocol);
   strcpy( transport.service, (char*)client_context->service);

   long result = casual_xatmi_receive( &transport);

   if ( result == AGAIN) return NGX_AGAIN;

   client_context->reply = copy_buffer( transport.payload, r);
   xatmi_tperrno = transport.code;
   client_context->code = transport.code;
   set_custom_header_in_headers_out( r, transport.header_out);

   free( transport.payload.data);

   return result == OK ? NGX_OK : NGX_ERROR;
}

void xatmi_cancel( ngx_http_xatmi_ctx_t* client_context, ngx_http_request_t* r)
{
   ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0,  "xatmi: canceling... descriptor [%d]", client_context->descriptor);

   casual_buffer_type transport;
   transport.header_in.size = 0; // No header needed
   transport.descriptor = client_context->descriptor;
   strcpy( transport.protocol, (char*)client_context->protocol);
   strcpy( transport.service, (char*)client_context->service);

   casual_xatmi_cancel( &transport);
}

ngx_int_t xatmi_max_service_length()
{
   return XATMI_SERVICE_NAME_LENGTH;
}

ngx_int_t get_header_length(ngx_http_request_t *r)
{
   ngx_list_part_t* part = &r->headers_in.headers.part;

   ngx_uint_t length = 0;
   ngx_uint_t i = 0;

   for (i = 0; /* void */ ; i++)
   {
      if (i >= part->nelts)
      {
        if (part->next == NULL)
        {
          /* The last part, search is done. */
          break;
        }

        part = part->next;
        i = 0;
      }
      length++;
   }
   return length;
}

ngx_int_t min( ngx_int_t a, ngx_int_t b)
{
   return (a > b ? b : a);
}

ngx_int_t copy_headers( ngx_http_request_t *r, header_data_type* header)
{
   ngx_list_part_t            *part;
   ngx_table_elt_t            *h;
   ngx_uint_t                  i;

   /*
    Get the first part of the list. There is usual only one part.
    */
   part = &r->headers_in.headers.part;
   h = part->elts;

   /*
    Headers list array may consist of more than one part,
    so loop through all of it
    */
   header_data_type* headerpointer = header;
   for (i = 0; /* void */ ; i++) {
      if (i >= part->nelts) {
         if (part->next == NULL) {
            /* The last part, search is done. */
            break;
         }

         part = part->next;
         h = part->elts;
         i = 0;
      }

      strncpy( headerpointer->key, (char*)h[i].key.data, h[i].key.len);
      headerpointer->key[ min(h[i].key.len, sizeof(headerpointer->key) - 1)] = '\0';
      strncpy( headerpointer->value, (char*)h[i].value.data, h[i].value.len);
      headerpointer->value[ min(h[i].value.len, sizeof(headerpointer->value) - 1)] = '\0';
      headerpointer++;
   }

   return NGX_OK;
}

void error_reporter(ngx_http_request_t* r, ngx_http_xatmi_ctx_t* client_context, ngx_int_t status, const char* message)
{
   client_context->reply.len = ngx_strlen( (u_char*)message);
   client_context->reply.data = ngx_palloc(r->pool,  client_context->reply.len);
   ngx_memcpy(client_context->reply.data,  (u_char*)message, client_context->reply.len);
   r->headers_out.status = status;
}

void xatmi_terminate( )
{
   //tpterm();
}

void error_handler(ngx_http_request_t* r, ngx_http_xatmi_ctx_t* client_context)
{
   u_char message[4096];
   ngx_memzero( message, sizeof( message)/ sizeof(u_char));

   // client_context->state == NGX_ERROR gör att xatmi_terminate (med tpterm) körs (i ett senare läge).
   // Initierar till NGX_OK, så får man istället i felhanteringen aktivt sätta till NGX_ERROR när tpterm önskas!
   client_context->state = NGX_OK;

   switch( xatmi_error_context )
   {
      // Antingen har tpinit smullit eller tpgetctxt.
   case cTPINIT:
      r->headers_out.status = NGX_HTTP_INTERNAL_SERVER_ERROR;
      // Tpterm önskas!
      client_context->state = NGX_ERROR;
      break;

   case cTPACALL:
      // Sätt lämplig HTTP-returkod och avgör om xatmi_terminate ska triggas
      switch( xatmi_tperrno )
      {
      case TPENOENT :
         r->headers_out.status = NGX_HTTP_NOT_FOUND;
         break;

      case TPELIMIT:
         r->headers_out.status = NGX_HTTP_SERVICE_UNAVAILABLE;
         break;

      case TPEINVAL:
      case TPEBLOCK:
      case TPGOTSIG:
      case TPEPROTO:
      case TPESYSTEM:
      case TPEOS:
      case TPEITYPE:
      case TPETRAN:
      case TPETIME:
         r->headers_out.status = NGX_HTTP_INTERNAL_SERVER_ERROR;
         break;

      default :
         r->headers_out.status = NGX_HTTP_INTERNAL_SERVER_ERROR;
      }
      break; //BREAK CASE TPACALL


   case cTPGETRPLY:

      // Sätt lämplig HTTP-returkod
      switch( xatmi_tperrno )
      {
      case TPESVCFAIL:
      case TPEINVAL:
      case TPETIME:
      case TPEBLOCK:
      case TPGOTSIG:
      case TPEPROTO:
      case TPESYSTEM:
      case TPEOS:
      case TPEOTYPE:
      case TPEBADDESC:
      case TPESVCERR:
         r->headers_out.status = NGX_HTTP_INTERNAL_SERVER_ERROR;
         break;

      default :
         r->headers_out.status = NGX_HTTP_INTERNAL_SERVER_ERROR;
      }
      break; //BREAK CASE TPGETREPLY

   case cTPALLOC:
      r->headers_out.status = NGX_HTTP_INSUFFICIENT_STORAGE;
      break;

   default:	// Oväntat scenario
      r->headers_out.status = NGX_HTTP_INTERNAL_SERVER_ERROR;
   }


   /* Lämpliga koder att mappa mot:
    * 4* = request side fel
    * 5* = server side fel
   NGX_HTTP_BAD_REQUEST 400
   NGX_HTTP_UNAUTHORIZED 401
   NGX_HTTP_FORBIDDEN 403
   NGX_HTTP_NOT_FOUND 404
   NGX_HTTP_NOT_ALLOWED 405
   NGX_HTTP_REQUEST_TIME_OUT 408
   NGX_HTTP_CONFLICT 409
   NGX_HTTP_LENGTH_REQUIRED 411
   NGX_HTTP_PRECONDITION_FAILED 412
   NGX_HTTP_REQUEST_ENTITY_TOO_LARGE 413
   NGX_HTTP_REQUEST_URI_TOO_LARGE 414
   NGX_HTTP_UNSUPPORTED_MEDIA_TYPE 415
   NGX_HTTP_RANGE_NOT_SATISFIABLE 416
   NGX_HTTP_CLOSE 444
   NGX_HTTP_NGINX_CODES 494
   NGX_HTTP_REQUEST_HEADER_TOO_LARGE 494
   NGX_HTTPS_CERT_ERROR 495
   NGX_HTTPS_NO_CERT 496
   NGX_HTTP_TO_HTTPS 497
   NGX_HTTP_CLIENT_CLOSED_REQUEST 499

   NGX_HTTP_INTERNAL_SERVER_ERROR 500
   NGX_HTTP_NOT_IMPLEMENTED 501
   NGX_HTTP_BAD_GATEWAY 502
   NGX_HTTP_SERVICE_UNAVAILABLE 503
   NGX_HTTP_GATEWAY_TIME_OUT 504
   NGX_HTTP_INSUFFICIENT_STORAGE 507
    */
}
