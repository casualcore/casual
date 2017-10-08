//
// Nginx
//
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_http_xatmi_common_declaration.h"
#include "xatmi_handler.h"


//
// Local configuration. Not needed.
//
typedef struct
{
   char server[30];
   ngx_flag_t enable;
} ngx_http_xatmi_loc_conf_t;

//
// Constants
//
const long cServiceLocationStart = sizeof("/casual/") - 1;

//
// Nginx "drivers"
//
static char* ngx_http_xatmi_setup(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);
static void* ngx_http_xatmi_create_loc_conf(ngx_conf_t* cf);
static char* ngx_http_xatmi_merge_loc_conf(ngx_conf_t* cf, void* parent, void* child);
static void ngx_http_xatmi_body_read(ngx_http_request_t* r);
static void ngx_http_xatmi_request_handler(ngx_event_t* ev);
static ngx_int_t ngx_xatmi_backend_init(ngx_http_xatmi_loc_conf_t* cf);

//
// Some handlers
//

static ngx_int_t bufferhandler( ngx_http_request_t* r, ngx_str_t* body);
static void restart_cycle( ngx_http_request_t* r);

//
// xatmi specific
//
static ngx_int_t call( ngx_http_request_t* r);
static ngx_int_t receive( ngx_http_request_t* r);

static ngx_command_t ngx_http_xatmi_commands[] = {
   {
      ngx_string("casual_pass"),
      NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
      ngx_http_xatmi_setup,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL
   },
   ngx_null_command
};

static ngx_http_module_t ngx_http_xatmi_module_ctx = {
   NULL, /* preconfiguration */
   NULL, /* postconfiguration */

   NULL, /* create main configuration */
   NULL, /* init main configuration */

   NULL, /* create server configuration */
   NULL, /* merge server configuration */

   ngx_http_xatmi_create_loc_conf, /* create location configuration */
   ngx_http_xatmi_merge_loc_conf /* merge location configuration */
};

ngx_module_t ngx_http_xatmi_module = { NGX_MODULE_V1,
   &ngx_http_xatmi_module_ctx, /* module context */
   ngx_http_xatmi_commands, /* module directives */
   NGX_HTTP_MODULE, /* module type */
   NULL, /* init master */
   NULL, /* init module */
   NULL, /* init process */
   NULL, /* init thread */
   NULL, /* exit thread */
   NULL, /* exit process */
   NULL, /* exit master */
   NGX_MODULE_V1_PADDING };

static ngx_int_t bufferhandler( ngx_http_request_t* r, ngx_str_t* body)
{
   //
   // we read data from r->request_body->bufs
   //
   if (r->request_body == NULL || r->request_body->bufs == NULL)
   {
      ngx_log_debug0( NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: No data in body.");
      return NGX_DONE;
   }
   else
   {
      ngx_log_debug0(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: Data in body.");
   }

   long content_length = r->headers_in.content_length_n;
   ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: content_length: %d", content_length);

   u_char* input =  ngx_pcalloc(r->pool, content_length);

   //
   // Any data from caller?
   //
   if (content_length > 0)
   {
      ngx_log_debug0(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: Copying body.");

      ngx_chain_t* bufs = r->request_body->bufs;

      u_char* p = input;
      ngx_int_t fileReading = 0;
      while ( 1)
      {
         u_int buffer_length = bufs->buf->last - bufs->buf->pos;
         ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: buffer_length: %d", buffer_length);
         if ( buffer_length > 0 )
         {
            ngx_memcpy(p, bufs->buf->start, buffer_length);
            ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: tmp p=%s", (const char*) p);
         }
         else
         {
            //
            // Lagrat temporärt på disk
            //
            if (bufs->buf->file_last > 0)
            {
               fileReading = 1;
               ngx_log_debug0(NGX_LOG_DEBUG_ALL, r->connection->log, 0,  "xatmi: reading from file");
               ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: bufs->buf->file_pos=%d", bufs->buf->file_pos);
               ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: bufs->buf->file_last=%d", bufs->buf->file_last);
               lseek( bufs->buf->file->fd, 0, SEEK_SET);
               if ( read(bufs->buf->file->fd, (void*)input, bufs->buf->file_last) == 0)
               {
                  ngx_log_debug0(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: error reading tempfile");
                  return NGX_ERROR;
               }
            }
            else
            {
               return NGX_ERROR;
            }
         }

         if (bufs->next == NULL)
         {
            p = NULL;

            if (bufs->buf->last_buf != 1 && !fileReading)
            {
               return NGX_AGAIN;
            }
            break;
         }
         bufs = bufs->next;
         //
         // Never mind the nullterminator
         //
         p += buffer_length;
      }

      body->data = ngx_pcalloc( r->pool, content_length);
      ngx_memcpy( body->data, input, content_length);
      body->len = content_length;

      ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: 1 body=%V", body);
   }
   else
   {
      content_length = 0;
      ngx_log_debug0(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: Empty body.");

      body->data = ngx_palloc(r->pool, content_length);
      ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: 2 body=%V", body);
   }
   return NGX_OK;
}

//
// call xatmi server asynchronous
//
static ngx_int_t call(ngx_http_request_t* r)
{
   ngx_http_xatmi_ctx_t* client_context = ngx_http_get_module_ctx(r, ngx_http_xatmi_module);
   ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: call Allocating buffer with len=%d", client_context->call_buffer.len);
   ngx_log_debug2(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: call protocol=%s, service=%s", client_context->protocol, client_context->service);

   return xatmi_call( client_context, r);
}

//
// Fetch the reply from xatmi
//
static ngx_int_t receive( ngx_http_request_t* r)
{
   ngx_http_xatmi_ctx_t* client_context = ngx_http_get_module_ctx(r, ngx_http_xatmi_module);

   ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: Waiting for answer: calling_descriptor=%d", client_context->calling_descriptor);
   ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: Waiting for answer: numberOfCalls=%d", client_context->numberOfCalls);
   ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: Waiting for answer: Initial size of client_context->reply_buffer: [%d]", client_context->reply_buffer.len);

   return xatmi_receive( client_context, r);

}

static ngx_int_t extract_information( ngx_http_request_t* r, ngx_http_xatmi_ctx_t* client_context)
{
   ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: uri=%V", &r->uri);

   u_char* start_of_service = &r->uri.data[ cServiceLocationStart];
   u_char* end_of_service = &r->uri.data[ r->uri.len];

   //
   // Copy some data to context for later use
   //
   //
   // Service
   //
   const int service_len = ((end_of_service - start_of_service) / sizeof (u_char));
   client_context->service = ngx_pcalloc(r->pool, service_len + 1);
   ngx_memcpy( client_context->service, start_of_service, service_len);
   client_context->service[ service_len] = '\0';

   //
   // Protocol
   //
   if ( r->headers_in.content_type)
   {
      const int protocol_len = r->headers_in.content_type->value.len;
      client_context->protocol = ngx_pcalloc(r->pool, protocol_len + 1);
      ngx_memcpy( client_context->protocol, r->headers_in.content_type->value.data, protocol_len);
      client_context->protocol[ protocol_len] = '\0';
   }
   else
   {
      client_context->protocol = ngx_pcalloc(r->pool, 1);
      client_context->protocol[ 0] = '\0';
   }

   ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: service=%s", client_context->service);
   ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: protocol=%s", client_context->protocol);

   //
   // Errorchecking
   //
   if ( service_len <= 0 || service_len > xatmi_max_service_length())
   {
      return NGX_HTTP_BAD_REQUEST;
   }

   ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: r->args=%V", &r->args);

   return NGX_OK;
}

static ngx_int_t ngx_xatmi_backend_handler( ngx_http_request_t* r)
{
   ngx_log_debug0(NGX_LOG_DEBUG_ALL, r->connection->log, 0,
      "xatmi: ngx_xatmi_backend_handler.\n");

   ngx_int_t rc;
   ngx_chain_t out;
   ngx_buf_t *b;

   ngx_http_xatmi_ctx_t* client_context = ngx_http_get_module_ctx(r, ngx_http_xatmi_module);

   //
   // Extract parameters from URL
   //
   if ( client_context == NULL)
   {
      client_context = ngx_pcalloc(r->pool, sizeof(ngx_http_xatmi_ctx_t));
      if (client_context == NULL)
      {
         return NGX_ERROR;
      }
      client_context->timeout = 1;
      client_context->state = NGX_OK;
      ngx_str_null( &client_context->call_buffer);

      //
      // Set unique context for this request
      //
      ngx_http_set_ctx( r, client_context, ngx_http_xatmi_module);

      rc = extract_information( r, client_context);
      if ( rc != NGX_OK)
      {
         const char* message = "{\n   \"error\" : \"INPUT DATA ERROR\"\n}";
         errorreporter(r, client_context, rc, message);
         goto produce_output;
      }

      //
      // GET and POST supported
      //
      if( !((r->method & NGX_HTTP_POST) || (r->method & NGX_HTTP_GET)))
      {
         const char* message = "{\n   \"error\" : \"INPUT DATA ERROR\"\n}";
         errorreporter(r, client_context, NGX_HTTP_NOT_ALLOWED, message);
         goto produce_output;
      }

      ngx_log_debug0(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: post http-check");

      r->headers_out.status = NGX_HTTP_OK;
   }

   if (client_context->state != NGX_OK)
   {
      const char* message = "{\n   \"error\" : \"RESOURCE FAILIURE\"\n}";
      errorreporter(r, client_context, NGX_HTTP_NOT_ALLOWED, message);
      goto produce_output;
   }

   //
   // Check if this is a phase for handling the reply from xatmi
   //
   if (client_context->calling_descriptor > 0)
   {
      goto receive;
   }

   if (client_context->call_buffer.data != NULL)
   {
      goto send;
   }

   //
   // Extract body
   //
   rc = ngx_http_read_client_request_body(r, ngx_http_xatmi_body_read);

   if (rc == NGX_AGAIN)
   {
      r->main->count++;
      client_context->waiting_more_body = 1;
      return NGX_DONE;
   }

   rc = bufferhandler(r, &client_context->call_buffer);
   ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: rc=%d", rc);

   if ( rc != NGX_OK && rc != NGX_DONE)
   {
      ngx_log_debug0(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: returning due to error");
      return rc;
   }

   ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: client_context->call_buffer=%V", &client_context->call_buffer);

   send:

   //
   // Make the call
   //
   client_context->calling_descriptor = call( r);
   if (client_context->calling_descriptor == NGX_ERROR)
   {
      //Errorhantering p.g.a. fel vid försök till tpacall eller bufferhantering m.a.a. detta
      errorhandler(r, client_context);
      goto produce_output;
   }
   else
   {
      restart_cycle( r);
      return NGX_DONE;
   }

   receive:

   rc = receive( r);
   if ( rc == NGX_AGAIN)
   {
      if (client_context->numberOfCalls > 600)
      {
         xatmi_cancel(client_context, r);

         errorhandler(r, client_context);
         goto produce_output;
      }

      restart_cycle( r);
      return NGX_DONE;
   }
   else if ( rc == NGX_ERROR)
   {
      xatmi_cancel(client_context, r);
      //Errorhantering p.g.a. fel vid försök till tpgetreply eller bufferhantering m.a.a. detta
      errorhandler(r, client_context);
   }

   produce_output:

   if (client_context->state != NGX_OK)
   {
      xatmi_terminate();
      // Se till att ny application context initieras pga tpterm nu har avslutat pågående context
   }


   ngx_log_debug0(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "Start producing output");
   //
   // Start preparing the answer
   //
   r->headers_out.content_type.len = ngx_strlen( client_context->protocol);
   r->headers_out.content_type.data = ngx_pcalloc( r->pool, r->headers_out.content_type.len + 1);
   ngx_sprintf( r->headers_out.content_type.data, "%s", client_context->protocol);

   //
   // Finish packaging the result
   //
   r->headers_out.content_length_n = client_context->reply_buffer.len;

   b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
   if (b == NULL )
   {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
         "xatmi: Failed to allocate response buffer.");
      return NGX_HTTP_INTERNAL_SERVER_ERROR;
   }

   out.buf = b;
   out.next = NULL;
   ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: client_context->reply_buffer=%V", &client_context->reply_buffer);

   b->pos = client_context->reply_buffer.data;
   b->last = client_context->reply_buffer.data + r->headers_out.content_length_n;

   b->memory = 1;
   b->last_buf = 1;

   rc = ngx_http_send_header(r);

   if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
   {
      ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: rc=%d", rc);
      return rc;
   }

   rc = ngx_http_output_filter(r, &out);

   ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: filtering result=%d", rc);
   ngx_log_debug0(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "xatmi: exiting ngx_xatmi_backend_handler");

   return rc == NGX_OK ? NGX_DONE : rc;
}

static char *
ngx_http_xatmi_setup(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
   ngx_http_core_loc_conf_t *clcf;
   ngx_http_xatmi_loc_conf_t *cglcf = conf;

   clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module) ;
   clcf->handler = ngx_xatmi_backend_handler;

   cglcf->enable = 1;

   return NGX_CONF_OK;
}

static void ngx_http_xatmi_body_read(ngx_http_request_t* r)
{
   ngx_http_xatmi_ctx_t* client_context = ngx_http_get_module_ctx(r, ngx_http_xatmi_module);

   ngx_log_debug1(NGX_LOG_DEBUG_ALL, r->connection->log, 0,
      "xatmi: http post read request body: count=%d", r->main->count);

   r->read_event_handler = ngx_http_request_empty_handler;

   r->main->count--;

   if (client_context->waiting_more_body )
   {
      client_context->waiting_more_body = 0;
      ngx_http_core_run_phases(r);
   }
}

static ngx_int_t ngx_xatmi_backend_init(ngx_http_xatmi_loc_conf_t* cglcf)
{

   strncpy(cglcf->server, "xatmiserver", 13);

   return NGX_OK;
}

static void *
ngx_http_xatmi_create_loc_conf(ngx_conf_t* cf)
{
   ngx_http_xatmi_loc_conf_t *conf;

   conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_xatmi_loc_conf_t));
   conf->enable = NGX_CONF_UNSET;
   return conf;
}

static char *
ngx_http_xatmi_merge_loc_conf(ngx_conf_t* cf, void* parent, void* child)
{
   ngx_http_xatmi_loc_conf_t *conf = child;

   if (conf->enable)
   {
      ngx_xatmi_backend_init(conf);
   }

   return NGX_CONF_OK ;
}

static void restart_cycle( ngx_http_request_t* r)
{
   ngx_http_xatmi_ctx_t* client_context = ngx_http_get_module_ctx(r, ngx_http_xatmi_module);
   //
   // Adjust timeout
   //
   if (client_context->numberOfCalls > 500)
   {
      client_context->timeout = 500;
   }
   else if (client_context->numberOfCalls > 100)
   {
      client_context->timeout = 100;
   }
   else if (client_context->numberOfCalls > 50)
   {
      client_context->timeout = 50;
   }

   r->main->count++;
   //
   // Start timer again
   //
   r->connection->read->handler = ngx_http_xatmi_request_handler;
   r->read_event_handler = ngx_http_core_run_phases;
   r->connection->read->log = r->connection->log;
   ngx_add_timer( r->connection->read, client_context->timeout );
   client_context->numberOfCalls++;

}
static void
ngx_http_xatmi_request_handler(ngx_event_t* ev)
{
   //
   // A copy of standard ngx_http_request_handler
   //
   ngx_connection_t    *c;
   ngx_http_request_t  *r;
   ngx_http_log_ctx_t  *ctx;

   c = ev->data;
   r = c->data;

   ctx = c->log->data;
   ctx->current_request = r;

   ngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0,
      "xatmi: http run request: \"%V?%V\"", &r->uri, &r->args);

   if (ev->write) {
      r->write_event_handler(r);

   } else {
      r->read_event_handler(r);
   }

   ngx_http_run_posted_requests(c);
}
