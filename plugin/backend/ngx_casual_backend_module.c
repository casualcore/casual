
#include <stddef.h>

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "xatmi.h"

typedef unsigned int u_int;
typedef unsigned char u_char;

static char* ngx_casual_backend(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static void* ngx_casual_backend_create_loc_conf(ngx_conf_t *cf);

static char* ngx_casual_backend_merge_loc_conf(ngx_conf_t *cf, void *parent,
      void *child);

static void ngx_casual_backend_input_post_read(ngx_http_request_t *r);

ngx_str_t handleError(ngx_http_request_t *r);

ngx_int_t bufferhandler( ngx_http_request_t *r, ngx_str_t* body);

typedef struct
{
   char server[30];
   ngx_flag_t enable;
} ngx_http_casual_loc_conf_t;

typedef struct {
    unsigned          waiting_more_body:1;
} ngx_casual_backend_ctx_t;


static ngx_int_t ngx_casual_backend_init(ngx_http_casual_loc_conf_t *cf);

static ngx_command_t ngx_casual_backend_commands[] = {
      {
            ngx_string("casual_caller"),
            NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
            ngx_casual_backend,
            NGX_HTTP_LOC_CONF_OFFSET,
            0,
            NULL
      },
      ngx_null_command
};

static ngx_http_module_t ngx_casual_backend_module_ctx = {
      NULL, /* preconfiguration */
      NULL, /* postconfiguration */

      NULL, /* create main configuration */
      NULL, /* init main configuration */

      NULL, /* create server configuration */
      NULL, /* merge server configuration */

      ngx_casual_backend_create_loc_conf, /* create location configuration */
      ngx_casual_backend_merge_loc_conf /* merge location configuration */
};

ngx_module_t ngx_casual_backend_module = { NGX_MODULE_V1,
      &ngx_casual_backend_module_ctx, /* module context */
      ngx_casual_backend_commands, /* module directives */
      NGX_HTTP_MODULE, /* module type */
      NULL, /* init master */
      NULL, /* init module */
      NULL, /* init process */
      NULL, /* init thread */
      NULL, /* exit thread */
      NULL, /* exit process */
      NULL, /* exit master */
      NGX_MODULE_V1_PADDING };

static ngx_int_t extractArgument( ngx_http_request_t *r, ngx_str_t argument, ngx_str_t* value, ngx_str_t* defaultValue)
{
   ngx_str_t s;

   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: ARGUMENT: %V", &argument);
   if (ngx_http_arg(r, argument.data, argument.len, &s) == NGX_OK)
   {
      value->len = s.len;
      value->data = ngx_pstrdup( r->pool, &s);
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: VALUE: %V", value);
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: Using selected value");
   }
   else
   {
      //
      // Has defaultvalue
      //
      if (defaultValue->len > 0)
      {
         //
         // Default
         //
         value = defaultValue;
         ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: Using default value");
      }
      else
      {
         return NGX_ERROR;
      }
   }

   return NGX_OK;
}

ngx_int_t bufferhandler( ngx_http_request_t *r, ngx_str_t* body)
{
   //
   // we read data from r->request_body->bufs
   //
   if (r->request_body == NULL || r->request_body->bufs == NULL)
   {
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: No data in body.");
      return NGX_DONE;
   }
   else
   {
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: Data in body.");
   }

   long content_length = r->headers_in.content_length_n;
   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: content_length: %d", content_length);
   //
   // Any data from caller?
   //
   if (content_length > 0)
   {
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: Copying body.");

      body->data = ngx_palloc(r->pool, content_length);
      body->len = content_length;
      memset((void*)body->data, '\0', content_length);
      ngx_chain_t* bufs = r->request_body->bufs;

      u_char* p = body->data;
      bool fileReading = true;
      while ( true)
      {
         u_int buffer_length = bufs->buf->last - bufs->buf->pos;
         ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: buffer_length: %d", buffer_length);
         if ( buffer_length > 0 )
         {
            ngx_memcpy(p, bufs->buf->start, buffer_length + 1);
            p[buffer_length] = '\0';
            ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: tmp p=%s", (const char*) p);

         }
         else
         {
            //
            // Lagrat temporärt på disk
            //
            if (bufs->buf->file_last > 0)
            {
               ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0,  "casual: reading from file");
               ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: bufs->buf->file_pos=%d", bufs->buf->file_pos);
               ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: bufs->buf->file_last=%d", bufs->buf->file_last);
               lseek( bufs->buf->file->fd, 0, SEEK_SET);
               if ( read(bufs->buf->file->fd, (void*)body->data, bufs->buf->file_last) == 0)
               {
                  ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: error reading tempfile");
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
         p += buffer_length - 1;
      }
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: 1 body=%V", body);
   }
   else
   {
      content_length = 0;
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: Empty body.");

      body->data = ngx_palloc(r->pool, content_length);
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: 2 body=%V", body);
   }
   return NGX_OK;
}

//
// call casual server asynchronous
//
int casual_call(ngx_http_request_t *r, ngx_str_t service, ngx_str_t protocol, ngx_str_t call_buffer)
{
   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: Allocating buffer protocol=%V with len=%d", &protocol, call_buffer.len);

   u_char* protocol_content = ngx_pcalloc(r->pool, protocol.len + 1);
   ngx_memcpy( protocol_content, protocol.data, protocol.len);
   protocol_content[protocol.len] = '\0';
   u_char* service_content = ngx_pcalloc(r->pool, service.len + 1);
   ngx_memcpy( service_content, service.data, service.len);
   service_content[service.len] = '\0';

   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: protocol_content=%s, service_content=%s", protocol_content, service_content);

   char* buffer = tpalloc( "X_OCTET", (const char*)protocol_content, call_buffer.len);
   int calling_descriptor = -1;

   if (buffer)
   {
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0,  "casual: Preparing for tpacall");
      strncpy(buffer, (const char*)call_buffer.data, call_buffer.len);

      //ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: buffer = %s", buffer);
      calling_descriptor = tpacall((const char*) service_content, buffer, call_buffer.len, 0);
   }

   tpfree( buffer);
   return calling_descriptor;
}

//
// Fetcb the reply from casual
//
ngx_int_t casual_receive( ngx_http_request_t *r, int calling_descriptor, ngx_str_t protocol, ngx_str_t* reply_buffer)
{
   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: Waiting for answer: cd=%d", calling_descriptor);
   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: Allocating buffer protocol=%V with len=1024", &protocol);

   u_char* protocol_content = ngx_pcalloc(r->pool, protocol.len + 1);
   ngx_memcpy( protocol_content, protocol.data, protocol.len);
   protocol_content[protocol.len] = '\0';

   char* buffer = tpalloc( "X_OCTET", (const char*)protocol_content, 1024);
   if (buffer)
   {

      int reply = tpgetrply(&calling_descriptor, &buffer, (long*)&reply_buffer->len, 0);
      if ( reply == -1)
      {
         tpfree( buffer);
         return NGX_ERROR;
      }

      reply_buffer->data = ngx_pcalloc(r->pool, reply_buffer->len);
      ngx_memcpy(reply_buffer->data, buffer, reply_buffer->len);
      tpfree( buffer);
   }
   return NGX_OK;
}

ngx_int_t handleArguments(ngx_http_request_t *r, ngx_str_t* service, ngx_str_t* protocol, ngx_str_t* asynchronous, ngx_str_t* calling_descriptor)
{

   static ngx_str_t noDefault = ngx_null_string;
   ngx_str_t serviceTag = ngx_string("service");
   if ( extractArgument( r, serviceTag, service, &noDefault) != NGX_OK)
   {
      return NGX_ERROR;
   }

   ngx_str_t protocolTag = ngx_string("protocol");
   ngx_str_t defaultProtocol = ngx_string("json");
   extractArgument( r, protocolTag, protocol, &defaultProtocol);

   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: SERVICE: %V", service);
   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: PROTOCOL: %V", protocol);

   ngx_str_t asynchronousTag = ngx_string("asynchronous");
   if (extractArgument( r, asynchronousTag, asynchronous, &noDefault) == NGX_OK)
   {
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: ASYNCHRONOUS: %V", asynchronous);
   }
   ngx_str_t calling_descriptorTag = ngx_string("calling_descriptor");
   if ( extractArgument( r, calling_descriptorTag, calling_descriptor, &noDefault) == NGX_OK)
   {
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: CALLING_DESCRIPTOR: %V", calling_descriptor);
   }

   return NGX_OK;
}

static ngx_int_t ngx_casual_backend_handler(ngx_http_request_t *r)
{
   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0,
         "casual: ngx_casual_backend_handler.\n");

   ngx_int_t rc;
   ngx_chain_t out;
   ngx_buf_t *b;
   ngx_str_t call_buffer;
   ngx_str_t reply_buffer;
   ngx_casual_backend_ctx_t* ctx;

   //
   // Only POST supported
   //
   if( ! ( r->method & (NGX_HTTP_POST)))
   {
      return NGX_HTTP_NOT_ALLOWED;
   }

   //
   // Read configuration
   //
   //ngx_http_casual_loc_conf_t* cglcf = ngx_http_get_module_loc_conf(r, ngx_casual_backend_module);

   r->headers_out.status = NGX_HTTP_OK;

   //
   // Extract parameters from URL
   //
   ngx_str_t service;
   ngx_str_t protocol;
   ngx_str_t asynchronous;
   ngx_str_t input_calling_descriptor;

   if ( handleArguments(r, &service, &protocol, &asynchronous, &input_calling_descriptor))
   {
      return NGX_ERROR;
   }

   ctx = ngx_pcalloc(r->pool, sizeof(ngx_casual_backend_ctx_t));
   if (ctx == NULL) {
       return NGX_ERROR;
   }

   /* set by ngx_pcalloc:
    *      ctx->waiting_more_body = 0;
    */

   ngx_http_set_ctx(r, ctx, ngx_casual_backend_module);

   //
   // Extract body
   //
   rc = ngx_http_read_client_request_body(r, ngx_casual_backend_input_post_read);

   if (rc == NGX_AGAIN)
   {
      r->main->count++;
      ctx->waiting_more_body = 1;
      return NGX_DONE;
   }

   rc = bufferhandler(r, &call_buffer);
   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: 3 rc=%d", rc);

   if ( rc != NGX_OK)
   {
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: 3 returning");
      return rc;
   }

   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: 3 call_buffer=%V", &call_buffer);

   if (r->headers_in.if_modified_since)
   {
      return NGX_HTTP_NOT_MODIFIED;
   }

   //
   // Start preparing the answer
   //
   r->headers_out.content_type.len = sizeof("application/") + protocol.len - 1;
   r->headers_out.content_type.data = ngx_pcalloc(r->pool, r->headers_out.content_type.len);
   ngx_sprintf( r->headers_out.content_type.data, "application/%V", &protocol);

   //
   // Call Casual
   //
   // TODO: Create own method
   //
   ngx_str_t error = ngx_null_string;

   int buffersize = 0;
   int calling_descriptor = casual_call(r, service, protocol, call_buffer);
   if (calling_descriptor == -1)
   {
      error = handleError(r);
      buffersize = error.len;
      goto finish;
   }

   rc = casual_receive( r, calling_descriptor, protocol, &reply_buffer);
   if (rc == NGX_ERROR)
   {
      error = handleError(r);
      buffersize = error.len;
      goto finish;
   }

   buffersize = reply_buffer.len;

finish:
   //
   // Finish packaging the result
   //
   r->headers_out.content_length_n = buffersize;

   b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
   if (b == NULL )
   {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
            "casual: Failed to allocate response buffer.");
      return NGX_HTTP_INTERNAL_SERVER_ERROR;
   }

   out.buf = b;
   out.next = NULL;

   u_char *text = ngx_palloc(r->pool, r->headers_out.content_length_n);
   if (text == NULL )
   {

      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
            "casual: Failed to allocate memory for text.");
      return NGX_HTTP_INTERNAL_SERVER_ERROR;
   }

   if (error.len > 0)
   {
      ngx_memcpy(text, error.data, error.len);
   }
   else
   {
      ngx_memcpy(text, reply_buffer.data, reply_buffer.len);
   }

   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, (char*) text);

   b->pos = text;
   b->last = text + r->headers_out.content_length_n;

   b->memory = 1;
   b->last_buf = 1;

   rc = ngx_http_send_header(r);

   if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
   {
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: rc=%d", rc);
      return rc;
   }

   rc = ngx_http_output_filter(r, &out);
   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: filterresult=%d", rc);
   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "casual: exiting ngx_casual_backend_handler");
   return rc;
}

static char *
ngx_casual_backend(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
   ngx_write_stderr("casual: ngx_casual_backend.\n");

   ngx_http_core_loc_conf_t *clcf;
   ngx_http_casual_loc_conf_t *cglcf = conf;

   clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module) ;
   clcf->handler = ngx_casual_backend_handler;

   cglcf->enable = 1;

   return NGX_CONF_OK ;
}

static void ngx_casual_backend_input_post_read(ngx_http_request_t *r)
{
   ngx_casual_backend_ctx_t     *ctx;

   ctx = ngx_http_get_module_ctx(r, ngx_casual_backend_module);

   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0,
                   "casual: http post read request body: count=%d", r->main->count);

    r->read_event_handler = ngx_http_request_empty_handler;

    r->main->count--;

    if (ctx->waiting_more_body) {
         ctx->waiting_more_body = 0;
         ngx_http_core_run_phases(r);
     }

}

static ngx_int_t ngx_casual_backend_init(ngx_http_casual_loc_conf_t *cglcf)
{

   strncpy(cglcf->server, "casualstyle", 12);
   ngx_write_stderr("casual: ngx_casual_backend_init.\n");

   return NGX_OK;
}

static void *
ngx_casual_backend_create_loc_conf(ngx_conf_t *cf)
{
   ngx_http_casual_loc_conf_t *conf;
   ngx_write_stderr("casual: ngx_casual_backend_create_loc_conf.\n");

   conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_casual_loc_conf_t));
   conf->enable = NGX_CONF_UNSET;
   return conf;
}

static char *
ngx_casual_backend_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
   //ngx_http_casual_loc_conf_t *prev = parent;
   ngx_http_casual_loc_conf_t *conf = child;
   ngx_write_stderr("casual: ngx_casual_backend_merge_loc_conf.\n");

   if (conf->enable)
      ngx_casual_backend_init(conf);

   return NGX_CONF_OK ;
}

ngx_str_t handleError(ngx_http_request_t *r)
{
   ngx_str_t error = ngx_null_string;
   error.len = strlen(tperrnostring(tperrno));
   error.data = ngx_pcalloc(r->pool, error.len);
   strncpy((char*)error.data, tperrnostring(tperrno), error.len);
   ngx_log_debug(NGX_LOG_ERR, r->connection->log, 0, "casual: error: %V", &error);
   r->headers_out.status = NGX_HTTP_BAD_REQUEST;
   return error;
}
