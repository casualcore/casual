//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "http/inbound/c/api.h"

// storage for session data used for one call
typedef struct ngx_http_casual_ctx
{
   ngx_http_request_t* http_request;
   ngx_str_t content_type;
   ngx_connection_t casual_connection;
   ngx_event_t read_event;
   ngx_event_t write_event;
   ngx_event_t timer;
   casual_http_inbound_handle_t casual_handle;
} ngx_http_casual_ctx_t;

typedef struct ngx_http_casual_main_conf
{
   ngx_log_t *log;
} ngx_http_casual_main_conf_t;

// location configuration supported
typedef struct ngx_http_casual_loc_conf
{
   ngx_str_t directive; // "service" or "forward"; TODO: change to enum
   ngx_str_t url_prefix; // default /
} ngx_http_casual_loc_conf_t;

// nginx integration api
static void* ngx_http_casual_create_main_conf( ngx_conf_t* configuration);
static char* ngx_http_casual_init_main_conf( ngx_conf_t* configuration, void* main_conf_ptr);
static void* ngx_http_casual_create_loc_conf( ngx_conf_t* configuration);
static char* ngx_http_casual_merge_loc_conf( ngx_conf_t* configuration, void* parent, void* child);
static char* ngx_http_casual_pass( ngx_conf_t* configuration, ngx_command_t* cmd, void* location_conf_ptr);
static void empty_handler( ngx_event_t* event);
static void read_event_handler( ngx_event_t* read_event);
static ngx_int_t ngx_http_casual_init_process( ngx_cycle_t* cycle);
static void ngx_http_casual_exit_process( ngx_cycle_t* cycle);
static void request_cleanup( void* data);
static ngx_int_t ngx_http_casual_request_handler( ngx_http_request_t* http_request);
static ngx_int_t process_request_parameters( ngx_http_request_t* http_request, ngx_http_casual_ctx_t* context);
static void ngx_http_casual_request_data_handler( ngx_http_request_t* http_request);

// interact with casual
static ngx_int_t send_request_to_casual( ngx_http_casual_ctx_t* context);
static void fetch_response_from_casual( ngx_http_casual_ctx_t* context);
static ngx_int_t send_response_upstream( ngx_http_casual_ctx_t* context);

// helpers
static void initilize_context( ngx_http_casual_ctx_t* context);
static ngx_chain_t* create_response( ngx_pool_t* pool, casual_http_inbound_string_t* payload);
static ngx_int_t get_header_length( ngx_http_request_t* http_request);
static ngx_int_t set_custom_header_in_headers_out( ngx_http_request_t* http_request, casual_http_inbound_headers_t* header);
static void copy_headers( ngx_http_request_t* http_request, casual_http_inbound_header_t* header);
static void free_headers( casual_http_inbound_header_t* header, ngx_int_t size);
static char* copy_from_ngx_str_t( ngx_str_t data);
static ngx_str_t copy_to_ngx_str_t( const char* const source, ngx_http_request_t* http_request);
static casual_http_inbound_string_t retrieve_service( ngx_str_t data, ngx_str_t url_prefix);
static casual_http_inbound_string_t to_string( ngx_str_t string);
static ngx_int_t buffer_size( ngx_chain_t* response);

static ngx_command_t
    ngx_http_casual_commands[] =
        {
            {
                // directive to set HTTP request handler for location
                ngx_string("casual_pass"),
                NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
                ngx_http_casual_pass,
                NGX_HTTP_LOC_CONF_OFFSET,
                0,   // not used;
                NULL // post handler
            },
            {
                // "service" or "forward"
                ngx_string("casual_directive"),
                NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
                ngx_conf_set_str_slot, // TODO: change to enum
                NGX_HTTP_LOC_CONF_OFFSET,
                offsetof(ngx_http_casual_loc_conf_t, directive),
                NULL // post handler
            },
            {
                // "url prefix" default "/""
                ngx_string("casual_url_prefix"),
                NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
                ngx_conf_set_str_slot, 
                NGX_HTTP_LOC_CONF_OFFSET,
                offsetof(ngx_http_casual_loc_conf_t, url_prefix),
                NULL // post handler
            },
            ngx_null_command};

static ngx_http_module_t
    ngx_http_casual_module_ctx =
        {
            NULL,                             // preconfiguration
            NULL,                             // postconfiguration
            ngx_http_casual_create_main_conf, // create main configuration
            ngx_http_casual_init_main_conf,   // init main configuration
            NULL,                             // create server configuration
            NULL,                             // merge server configuration
            ngx_http_casual_create_loc_conf,  // allocates and initializes location-scope struct
            ngx_http_casual_merge_loc_conf    // sets location-scope struct values from outer scope if left unset in location scope
};

ngx_module_t
    ngx_http_casual_module =
        {
            NGX_MODULE_V1,
            &ngx_http_casual_module_ctx,  // module callbacks
            ngx_http_casual_commands,     // module configuration callbacks
            NGX_HTTP_MODULE,              // module type is HTTP
            NULL,                         // init_master
            NULL,                         // init_module
            ngx_http_casual_init_process, // init_process
            NULL,                         // init_thread
            NULL,                         // exit_thread
            ngx_http_casual_exit_process, // exit_process
            NULL,                         // exit_master
            NGX_MODULE_V1_PADDING};

static void* ngx_http_casual_create_main_conf( ngx_conf_t* configuration)
{
   ngx_conf_log_error( NGX_LOG_NOTICE, configuration, 0, "casual: %s", __FUNCTION__);
   ngx_http_casual_main_conf_t* main_configuration = ngx_pcalloc( configuration->pool, sizeof(ngx_http_casual_main_conf_t));
   if (main_configuration != NULL)
   { // unset fields
   }

   return main_configuration;
}

static char* ngx_http_casual_init_main_conf( ngx_conf_t* configuration, void* main_conf_ptr)
{
   ngx_conf_log_error( NGX_LOG_NOTICE, configuration, 0, "casual: %s", __FUNCTION__);
   // ngx_http_casual_main_conf_t *main_configuration = main_conf_ptr;
   return NGX_CONF_OK;
}

static void* ngx_http_casual_create_loc_conf( ngx_conf_t* configuration)
{
   ngx_conf_log_error( NGX_LOG_NOTICE, configuration, 0, "casual: %s", __FUNCTION__);
   ngx_http_casual_loc_conf_t* location_configuration = ngx_pcalloc( configuration->pool, sizeof(ngx_http_casual_loc_conf_t));
   if( location_configuration != NULL)
   { // unset fields
     // location_configuration->directive = NGX_CONF_UNSET_UINT;
   }
   return location_configuration;
}

static char* ngx_http_casual_merge_loc_conf( ngx_conf_t* configuration, void* parent, void* child)
{
   ngx_conf_log_error( NGX_LOG_NOTICE, configuration, 0, "casual: %s", __FUNCTION__);
   ngx_http_casual_loc_conf_t* prev = parent;
   ngx_http_casual_loc_conf_t* location_configuration = child;
   ngx_conf_merge_str_value( location_configuration->directive, prev->directive, /*default*/ "service"); 
   ngx_conf_merge_str_value( location_configuration->url_prefix, prev->url_prefix, /*default*/ "/"); 

   return NGX_CONF_OK;
}

static char* ngx_http_casual_pass( ngx_conf_t* configuration, ngx_command_t* cmd, /*ngx_http_casual_loc_conf_t*/ void* location_conf_ptr)
{
   ngx_conf_log_error( NGX_LOG_NOTICE, configuration, 0, "casual: %s", __FUNCTION__);
   ngx_http_core_loc_conf_t* http_location_configuration = ngx_http_conf_get_module_loc_conf( configuration, ngx_http_core_module);
   http_location_configuration->handler = ngx_http_casual_request_handler; // sets HTTP request handler
   return NGX_CONF_OK;
}

static void empty_handler( ngx_event_t* event)
{
   ngx_log_error( NGX_LOG_NOTICE, event->log, 0, "casual: %s, %s, active = ", __FUNCTION__, event->write ? "WRITE" : "READ", event->active);
}

static void read_event_handler( ngx_event_t* read_event)
{
   ngx_connection_t* connection = read_event->data;
   ngx_log_error( NGX_LOG_NOTICE, connection->log, 0, "casual: %s", __FUNCTION__);

   ngx_http_casual_ctx_t* context = connection->data;
   fetch_response_from_casual( context);
}

static ngx_int_t ngx_http_casual_init_process( ngx_cycle_t* cycle)
{
   ngx_log_error( NGX_LOG_NOTICE, cycle->log, 0, "casual: %s", __FUNCTION__);

   ngx_http_casual_main_conf_t* main_configuration = ngx_http_cycle_get_module_main_conf( cycle, ngx_http_casual_module);

   if( ! main_configuration)
   {
      ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "casual: !main_configuration");
      return NGX_ERROR;
   }

   return NGX_OK;
}

static void ngx_http_casual_exit_process( ngx_cycle_t* cycle)
{
   ngx_log_error( NGX_LOG_NOTICE, cycle->log, 0, "casual: %s", __FUNCTION__);
}

static ngx_int_t ngx_http_casual_request_handler( ngx_http_request_t* http_request)
{
   ngx_log_error( NGX_LOG_NOTICE, http_request->connection->log, 0, "casual: %s pid=%d", __FUNCTION__, ngx_pid);

   if( ! (http_request->method & (NGX_HTTP_GET | NGX_HTTP_POST))) // TODO: all methods
   {
      return NGX_HTTP_NOT_ALLOWED;
   }
   ngx_http_casual_ctx_t* context = ngx_pcalloc( http_request->pool, sizeof(ngx_http_casual_ctx_t)); // NOTE: zero-initialized
   if( context == NULL)
   {
      return NGX_HTTP_INTERNAL_SERVER_ERROR;
   }
   initilize_context( context);
   ngx_http_set_ctx( http_request, context, ngx_http_casual_module); // makes context retrievable from http_request with ngx_http_get_module_ctx(http_request, ngx_http_casual_module)
   context->http_request = http_request;

   ngx_int_t return_code;
   if( ( return_code = process_request_parameters( http_request, context)) != NGX_OK)
   {
      return return_code;
   }

   if( ( http_request->method & NGX_HTTP_GET) && ngx_http_discard_request_body(http_request) != NGX_OK)
   {
      return NGX_HTTP_INTERNAL_SERVER_ERROR;
   }

   http_request->request_body_in_single_buf = 1;
   // make sure we do not buffer. -> not no buffering -> we want buffering! -> the whole body when the callback is called.
   http_request->request_body_no_buffering = 0;

   return_code = ngx_http_read_client_request_body( http_request, ngx_http_casual_request_data_handler); // delegates to body handler callback
   if( return_code >= NGX_HTTP_SPECIAL_RESPONSE)
   {
      return return_code;
   }
   return NGX_DONE; // doesn't destroy request until ngx_http_finalize_request is called
}

static ngx_int_t process_request_parameters( ngx_http_request_t* http_request, ngx_http_casual_ctx_t* context)
{
   ngx_log_error( NGX_LOG_NOTICE, http_request->connection->log, 0, "casual: url = '%V'", &http_request->unparsed_uri);

   // TODO: get more parameters to pass to casual call
   if( http_request->headers_in.content_type)
   {
      ngx_log_error( NGX_LOG_NOTICE, http_request->connection->log, 0, "casual: content-type = '%V'", &http_request->headers_in.content_type->value);
      context->content_type.data = http_request->headers_in.content_type->value.data;
      context->content_type.len = http_request->headers_in.content_type->value.len;
   }

   if( http_request->headers_in.content_length_n)
   {
      ngx_log_error(NGX_LOG_NOTICE, http_request->connection->log, 0, "casual: content-length = '%O'", http_request->headers_in.content_length_n);
   }

   return NGX_OK;
}

static void ngx_http_casual_request_data_handler( ngx_http_request_t* http_request)
{
   ngx_log_error( NGX_LOG_NOTICE, http_request->connection->log, 0, "casual: %s", __FUNCTION__);

   ngx_http_casual_ctx_t* context = ngx_http_get_module_ctx( http_request, ngx_http_casual_module);
   
   // since we've set http_request->request_body_in_single_buf = 1; and http_request->request_body_no_buffering = 0;
   // we know that we'll get a single buffer/file and don't need to check last_buf and such.
   for( ngx_chain_t* cl = http_request->request_body->bufs; cl; cl = cl->next)
   {
      off_t buffer_size = ngx_buf_size(cl->buf);

      if( cl->buf->in_file || cl->buf->temp_file) // if buffered in file, then read entire file into a buffer
      {
         ngx_log_error( NGX_LOG_NOTICE, context->http_request->connection->log, 0, "casual: buffer in file");
         ngx_buf_t* buf = ngx_create_temp_buf( http_request->pool, buffer_size);
         ssize_t bytes_read = ngx_read_file( cl->buf->file, buf->pos, buffer_size, cl->buf->file_pos);
         if( bytes_read != (ssize_t)buffer_size)
         {
            ngx_log_error(NGX_LOG_ERR, http_request->connection->log, 0, "casual: error reading tempfile; return_code=%zu", bytes_read);
            ngx_http_finalize_request(http_request, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
         }
         casual_http_inbound_push_payload( &context->casual_handle, buf->pos, buffer_size);
      }
      else
      {
         casual_http_inbound_push_payload( &context->casual_handle, cl->buf->pos, buffer_size);
      }
   }

   ngx_int_t return_code = send_request_to_casual( context);
   if (return_code == NGX_ERROR)
   {
      ngx_http_finalize_request( http_request, NGX_HTTP_INTERNAL_SERVER_ERROR);
   }
   else if (return_code == NGX_OK)
   { 
      // finalize right away on OK; TODO: enable this case and prevent NULL errors
      // send_request_to_casual will never return NGX_OK, so why this?
      ngx_http_finalize_request( http_request, send_response_upstream( context));
   }
   // else 
   // NGX_AGAIN to finalize later
   // ngx_http_finalize_request is only needed if we wan't do to a premature http-finalization?

   // everything went OK and nginx will finalize? 
}

static ngx_int_t send_request_to_casual( ngx_http_casual_ctx_t* context)
{
   ngx_log_error( NGX_LOG_NOTICE, context->http_request->connection->log, 0, "casual: %s", __FUNCTION__);

   ngx_http_casual_loc_conf_t* location_configuration = ngx_http_get_module_loc_conf(context->http_request, ngx_http_casual_module);
   ngx_http_casual_main_conf_t* main_configuration = ngx_http_get_module_main_conf(context->http_request, ngx_http_casual_module);
   if( ! location_configuration || ! main_configuration)
   {
      ngx_log_error( NGX_LOG_ERR, context->http_request->connection->log, 0, "casual: location_configuration == NULL || main_configuration == NULL");
      return NGX_ERROR;
   }

   ngx_pool_cleanup_t* cleanup = ngx_pool_cleanup_add( context->http_request->pool, 0);
   if( ! cleanup)
   {
      return NGX_ERROR;
   }
   cleanup->handler = request_cleanup; // set request's cleanup callback
   cleanup->data = context;     // handler argument

   casual_http_inbound_request_t request;
   request.method = to_string( context->http_request->method_name);
   request.url = to_string( context->http_request->unparsed_uri);
   request.service = retrieve_service( context->http_request->uri, location_configuration->url_prefix);
   
   // handle headers
   ngx_int_t headersize = get_header_length(context->http_request);
   casual_http_inbound_header_t* headers = 0L;
   if( headersize > 0)
   {
      headers = (casual_http_inbound_header_t *)malloc(headersize * sizeof(casual_http_inbound_header_t));
      copy_headers(context->http_request, headers);

      request.headers.data = headers;
      request.headers.size = headersize;
   }

   // set values in casual context
   casual_http_inbound_request_set( &context->casual_handle, &request);

   // free headers once copied
   free_headers( headers, headersize);

   // if active choosen forward then forward else service
   ngx_int_t directive = ngx_strcmp( location_configuration->directive.data, "forward") == 0 ? forward : service;

   casual_http_inbound_call( &context->casual_handle, directive); // Context(directive, request)

   // if response not immediately available
   ngx_connection_t* connection = &context->casual_connection;
   connection->data = context;
   connection->read = &context->read_event;
   connection->write = &context->write_event;
   connection->fd = context->casual_handle.fd;
   connection->read->data = connection; // nginx expects ngx_connection_t as event data
   connection->log = context->http_request->connection->log;
   connection->read->log = context->http_request->connection->log;
   connection->write->index = NGX_INVALID_INDEX;
   connection->write->data = connection;
   connection->write->write = 1;
   connection->write->log = context->http_request->connection->log;
   connection->read->index = NGX_INVALID_INDEX;

   connection->read->handler = read_event_handler; // called when fd is readable
   connection->write->handler = empty_handler;     // ignore when fd is writable

   if( ngx_handle_read_event( connection->read, 0) != NGX_OK)
   {
      ngx_log_error( NGX_LOG_ERR, context->http_request->connection->log, 0, "casual: %s, %s", __FUNCTION__, "ngx_handle_write_event/ngx_handle_read_event");
      return NGX_ERROR;
   }

   return NGX_AGAIN;
}

static void fetch_response_from_casual( ngx_http_casual_ctx_t* context)
{
   ngx_log_error( NGX_LOG_NOTICE, context->http_request->connection->log, 0, "casual: %s", __FUNCTION__);

   int return_code = casual_http_inbound_receive( &context->casual_handle);
   if( return_code == NGX_OK)
   {
      // remove event handler for read event
      if( context->read_event.active)
      {
         context->read_event.handler = empty_handler;
         if( NGX_OK != ngx_del_event(&context->read_event, NGX_READ_EVENT, NGX_CLOSE_EVENT))
         {
            ngx_log_error( NGX_LOG_ERR, context->http_request->connection->log, 0, "casual: %s, ngx_del_event(read_event, NGX_READ_EVENT, NGX_CLOSE_EVENT)", __FUNCTION__);
         }
      }

      if( context->timer.timer_set)
      {
         ngx_del_timer(&context->timer);
      }

      // called in cleanup instead (doesn't really need a copy_from_ngx_str_t of data now)
      ngx_http_finalize_request( context->http_request, send_response_upstream(context));
   }
}

static ngx_int_t send_response_upstream( ngx_http_casual_ctx_t* context)
{
   ngx_http_request_t* http_request = context->http_request;
   ngx_log_error( NGX_LOG_NOTICE, http_request->connection->log, 0, "casual: %s", __FUNCTION__);

   // retreive actual reply data
   casual_http_inbound_reply_t reply;
   casual_http_inbound_reply_get( &context->casual_handle, &reply);

   ngx_chain_t* response = create_response( context->http_request->pool, &reply.payload);
   http_request->headers_out.content_length_n = buffer_size( response);

   http_request->headers_out.status = reply.code;

   if( reply.content_type.size > 0)
   {
      http_request->headers_out.content_type.len = reply.content_type.size;
      http_request->headers_out.content_type.data = ngx_palloc( http_request->pool, http_request->headers_out.content_type.len);
      ngx_memcpy( http_request->headers_out.content_type.data, reply.content_type.data, http_request->headers_out.content_type.len);
   }
   else
   {
      // same content-type as incomming
      http_request->headers_out.content_type.data = ngx_pstrdup( http_request->pool, &context->content_type);
      http_request->headers_out.content_type.len = context->content_type.len;
   }

   set_custom_header_in_headers_out( http_request, &reply.headers);

   // we can only return NGX_OK, NGX_ERROR and such, since this is one of the arguments to ngx_http_finalize_request (above)
   ngx_int_t result_code = ngx_http_send_header( http_request);

   if( result_code == NGX_ERROR || result_code > NGX_OK || http_request->header_only) 
   {
      ngx_log_debug( NGX_LOG_DEBUG_ALL, http_request->connection->log, 0, "casual: ngx_http_send_header() == %d", result_code);
      return result_code;
   }

   if( ( result_code = ngx_http_output_filter(http_request, response)) != NGX_OK)
   {
      ngx_log_debug( NGX_LOG_DEBUG_ALL, http_request->connection->log, 0, "casual: ngx_http_output_filter() == %d", result_code);
   }

   return result_code;
}

static void request_cleanup( void* data)
{
   ngx_http_casual_ctx_t* context = data;
   ngx_log_error( NGX_LOG_NOTICE, context->http_request->connection->log, 0, "casual: %s", __FUNCTION__);
 
   if( context->read_event.active)
   {
      // context->read_event.handler = empty_handler;
      if( NGX_OK != ngx_del_event(&context->read_event, NGX_READ_EVENT, NGX_CLOSE_EVENT))
      {
         ngx_log_error(NGX_LOG_ERR, context->http_request->connection->log, 0, "casual: %s, ngx_del_event(read_event, NGX_READ_EVENT, NGX_CLOSE_EVENT)", __FUNCTION__);
      }
   }
   if( context->write_event.active)
   {
      // context->write_event.handler = empty_handler;
      if( NGX_OK != ngx_del_event(&context->write_event, NGX_WRITE_EVENT, NGX_CLOSE_EVENT))
      {
         ngx_log_error(NGX_LOG_ERR, context->http_request->connection->log, 0, "casual: %s, ngx_del_event(write_event, NGX_WRITE_EVENT, NGX_CLOSE_EVENT)", __FUNCTION__);
      }
   }
   if( context->timer.timer_set)
   {
      ngx_del_timer(&context->timer);
   }

   casual_http_inbound_deallocate( &context->casual_handle);
}

static void initilize_context( ngx_http_casual_ctx_t* context)
{
   context->casual_handle.fd = 0;
   context->casual_handle.context_holder = NULL;
}

static ngx_int_t get_header_length( ngx_http_request_t* http_request)
{
   ngx_list_part_t* part = &http_request->headers_in.headers.part;
   ngx_uint_t length = 0;

   while( part)
   {
      length += part->nelts; // might be "of by one"? 
      part = part->next;   
   }

   /*
   for( i = 0; ; i++)
   {
      if( i >= part->nelts)
      {
         if( part->next == NULL)
         {
            break;
         }

         part = part->next;
         i = 0;
      }
      length++;
   }
   */

   return length;
}

static void copy_headers( ngx_http_request_t* http_request, casual_http_inbound_header_t* header)
{
   /*
    Get the first part of the list. There is usual only one part.
    */
   ngx_list_part_t* part = &http_request->headers_in.headers.part;

   while( part)
   {
      ngx_table_elt_t* element = part->elts;
   
      while( element !=  (ngx_table_elt_t*)part->elts + part->nelts)
      {
         header->key =  copy_from_ngx_str_t( element->key);
         header->value =  copy_from_ngx_str_t( element->value);
         ++header;
         ++element;
      }
      part = part->next;
   }


   /*
    Headers list array may consist of more than one part,
    so loop through all of it
    */
   /*

   for( ngx_uint_t i = 0; ; i++)
   {
      if( i >= part->nelts)
      {
         if( ! part->next)
         {
            break;
         }

         part = part->next;
         h = part->elts;
         i = 0;
      }

      header->key = copy_from_ngx_str_t( h[i].key);
      header->value = copy_from_ngx_str_t( h[i].value);
      header++;
   }
   */
}

static void free_headers( casual_http_inbound_header_t* header, ngx_int_t size)
{
   for( ngx_int_t i = 0; i < size; ++i)
   {
      free( header[i].key);
      free( header[i].value);
   }
   free( header);
   header = 0L;
}

static ngx_int_t set_custom_header_in_headers_out( ngx_http_request_t* http_request, casual_http_inbound_headers_t* header)
{
   for( size_t i = 0; i < header->size; i++)
   {
      /*
      All we have to do is just to allocate the header...
      */
      ngx_table_elt_t* element = ngx_list_push( &http_request->headers_out.headers);
      if ( ! element) {
         return NGX_ERROR;
      }

      /*
      ... setup the header key ...
      */
      element->key = copy_to_ngx_str_t( header->data[i].key, http_request);
      ngx_log_debug1( NGX_LOG_DEBUG_ALL, http_request->connection->log, 0, "xatmi: key [%V]", &element->key);


      /*
      ... and the value.
      */
      element->value = copy_to_ngx_str_t( header->data[i].value, http_request);
      ngx_log_debug1( NGX_LOG_DEBUG_ALL, http_request->connection->log, 0, "xatmi: value [%V]", &element->value);


      /*
      Mark the header as not deleted.
      */
      element->hash = 1;
   }
    return NGX_OK;
}

static ngx_int_t buffer_size( ngx_chain_t* response)
{
   ngx_int_t result = 0;
   for( ngx_chain_t* current = response; current; current = current->next)
   {
      result += ngx_buf_size( current->buf);
      if( current->next == NULL)
      {
         current->buf->last_in_chain = 1;
         current->buf->last_buf = 1;
      }
   }

   return result;
}

static ngx_chain_t* create_response( ngx_pool_t* pool, casual_http_inbound_string_t* payload)
{
   if( payload->data && payload->size)
   {
      ngx_buf_t* buffer = ngx_create_temp_buf( pool, payload->size);
      buffer->last = ngx_cpymem( buffer->pos, payload->data, payload->size); 
      buffer->memory = 1;
      
      // we need to set that this is the one and only buffer and buffer window?
      buffer->last_buf = 1;
      buffer->last_in_chain = 1;

      ngx_chain_t* response = ngx_alloc_chain_link( pool);
      response->next = NULL;
      response->buf = buffer;
      return response;
   }

   return NULL;
}

static char* copy_from_ngx_str_t( ngx_str_t data)
{
   char* result = (char*)malloc( data.len + 1);
   strncpy( result, (const char *)data.data, data.len);
   result[ data.len] = '\0';
   return result;
}

static ngx_str_t copy_to_ngx_str_t( const char* const source, ngx_http_request_t* http_request)
{
   ngx_str_t destination;
   ngx_int_t size = strlen( source);
   destination.data = ngx_pcalloc( http_request->pool,  size + 1);
   ngx_memcpy( destination.data, source, size);
   destination.len = size;
   destination.data[ size] = '\0';
   return destination;
}

static casual_http_inbound_string_t retrieve_service( ngx_str_t data, ngx_str_t url_prefix)
{
   casual_http_inbound_string_t result;
   result.size = data.len - url_prefix.len;
   result.data = (char*)malloc( result.size);
   strncpy( result.data, (const char *)data.data + url_prefix.len, result.size);
   return result;
}

static casual_http_inbound_string_t to_string( ngx_str_t string)
{
   casual_http_inbound_string_t result;
   result.data = (char*)string.data;
   result.size = string.len;
   return result;
}
