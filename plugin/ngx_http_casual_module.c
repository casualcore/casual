/*
 * Copyright (C) Evan Miller
 */

#include <stddef.h>

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "xatmi.h"

typedef unsigned int u_int;
typedef unsigned char u_char;

static char* ngx_http_casual(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static void* ngx_http_casual_create_loc_conf(ngx_conf_t *cf);

static char* ngx_http_casual_merge_loc_conf(ngx_conf_t *cf, void *parent,
      void *child);

static void ngx_http_casual_input_post_read(ngx_http_request_t *r);

char* handleError(ngx_http_request_t *r);

typedef struct
{
   char server[30];
   ngx_flag_t enable;
} ngx_http_casual_loc_conf_t;

static ngx_int_t ngx_http_casual_init(ngx_http_casual_loc_conf_t *cf);

static ngx_command_t ngx_http_casual_commands[] = {
      {
            ngx_string("casual"),
            NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
            ngx_http_casual,
            NGX_HTTP_LOC_CONF_OFFSET,
            0,
            NULL
      },
      ngx_null_command
};

static ngx_http_module_t ngx_http_casual_module_ctx = {
      NULL, /* preconfiguration */
      NULL, /* postconfiguration */

      NULL, /* create main configuration */
      NULL, /* init main configuration */

      NULL, /* create server configuration */
      NULL, /* merge server configuration */

      ngx_http_casual_create_loc_conf, /* create location configuration */
      ngx_http_casual_merge_loc_conf /* merge location configuration */
};

ngx_module_t ngx_http_casual_module = { NGX_MODULE_V1,
      &ngx_http_casual_module_ctx, /* module context */
      ngx_http_casual_commands, /* module directives */
      NGX_HTTP_MODULE, /* module type */
      NULL, /* init master */
      NULL, /* init module */
      NULL, /* init process */
      NULL, /* init thread */
      NULL, /* exit thread */
      NULL, /* exit process */
      NULL, /* exit master */
      NGX_MODULE_V1_PADDING };

static ngx_int_t extractArgument( ngx_http_request_t *r, const char* const argument, u_char* value, const char* const defaultValue)
{
   ngx_str_t s;
   u_char* p;
   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, argument);
   if (ngx_http_arg(r, (u_char*) argument, strlen(argument), &s) == NGX_OK)
   {
      p = ngx_copy(value, s.data, s.len );
      *p = '\0';
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "Using selected value");
   }
   else
   {
      //
      // Has defaultvalue
      //
      if (defaultValue)
      {
         //
         // Default
         //
         ngx_memcpy(value, (u_char*) defaultValue, sizeof(defaultValue) - 1);
         value[sizeof(defaultValue) - 1] = '\0';
         ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, "Using default value");
      }
   }

   return NGX_OK;
}

static ngx_int_t ngx_http_casual_handler(ngx_http_request_t *r)
{
   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0,
         "ngx_http_casual_handler.\n");

   ngx_int_t rc;
   ngx_chain_t out;
   ngx_buf_t *b;
   u_char *body = 0;

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
   //ngx_http_casual_loc_conf_t *cglcf;
   //cglcf = ngx_http_get_module_loc_conf(r, ngx_http_casual_module);

   r->headers_out.status = NGX_HTTP_OK;

   //
   // Extract parameters from URL
   //
   u_char service[100];
   if ( extractArgument( r, "service", service, 0) != NGX_OK)
   {
      return NGX_ERROR;
   }

   u_char protocol[10];
   extractArgument( r, "protocol", protocol, "JSON");

   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, (char*) service);
   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, (char*) protocol);

   //
   // Extract body
   //
   rc = ngx_http_read_client_request_body(r, ngx_http_casual_input_post_read);

   //
   // we read data from r->request_body->bufs
   //
   if (r->request_body == NULL || r->request_body->bufs == NULL)
   {
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, (char*) "No data in body.");
   }
   else
   {
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, (char*) "Data in body.");
   }

   long content_length = r->headers_in.content_length_n;
   //
   // Any data from caller?
   //
   if (content_length > 0)
   {
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, (char*) "Copying body.");

      body = ngx_palloc(r->pool, content_length);
      ngx_memcpy(body, r->request_body->bufs->buf->start, content_length);
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, (char*) body);
   }
   else
   {
      content_length = 0;
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, (char*) "Empty body.");

      body = ngx_palloc(r->pool, content_length);
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, (char*) body);
   }

   if (r->headers_in.if_modified_since)
   {
      return NGX_HTTP_NOT_MODIFIED;
   }

   //
   // Start preparing the answer
   //
   r->headers_out.content_type.len = sizeof("application/json") - 1;
   r->headers_out.content_type.data = (u_char *) "application/json";

   //
   // Call Casual
   //
   // TODO: Create own method
   //
   long buffersize = 0;
   char* error = NULL;

   char* buffer = tpalloc(X_OCTET, (char*)protocol, 1024);

   strncpy(buffer, (const char*)body, content_length + 1);
   buffer[content_length] = '\0';

   if (!buffer)
   {
      error = handleError(r);
      buffersize = strlen(error);
   }
   else
   {
      int cd1 = 0;

      //
      // Do the call
      //
      if ((cd1 = tpacall((char*) service, buffer, content_length, 0)) == -1)
      {
         error = handleError(r);
         buffersize = strlen(error);
      }
      else
      {
         //
         // Handle the reply
         //
         if (tpgetrply(&cd1, &buffer, &buffersize, 0) == -1)
         {
            error = handleError(r);
            buffersize = strlen(error);
         }
      }
   }

   //
   // Finish packaging the result
   //
   r->headers_out.content_length_n = buffersize;

   b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
   if (b == NULL )
   {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
            "Failed to allocate response buffer.");
      tpfree(buffer);
      return NGX_HTTP_INTERNAL_SERVER_ERROR;
   }

   out.buf = b;
   out.next = NULL;

   u_char *text = ngx_palloc(r->pool, r->headers_out.content_length_n);
   if (text == NULL )
   {

      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
            "Failed to allocate memory for text.");
      tpfree(buffer);
      return NGX_HTTP_INTERNAL_SERVER_ERROR;
   }

   if (error)
   {
      ngx_memcpy(text, error, r->headers_out.content_length_n);
   }
   else
   {
      ngx_memcpy(text, buffer, r->headers_out.content_length_n);
   }

   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, (char*) text);

   b->pos = text;
   b->last = text + r->headers_out.content_length_n;

   b->memory = 1;
   b->last_buf = 1;

   tpfree(buffer);

   rc = ngx_http_send_header(r);

   if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
   {
      return rc;
   }

   return ngx_http_output_filter(r, &out);
}

static char *
ngx_http_casual(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
   ngx_write_stderr("ngx_http_casual.\n");

   ngx_http_core_loc_conf_t *clcf;
   ngx_http_casual_loc_conf_t *cglcf = conf;

   clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module) ;
   clcf->handler = ngx_http_casual_handler;

   cglcf->enable = 1;

   return NGX_CONF_OK ;
}

static void ngx_http_casual_input_post_read(ngx_http_request_t *r)
{

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http post read request body");

    r->read_event_handler = ngx_http_request_empty_handler;
}

static ngx_int_t ngx_http_casual_init(ngx_http_casual_loc_conf_t *cglcf)
{

   u_int i = 0;
   strncpy(cglcf->server, "casualstyle", 12);
   ngx_write_stderr("ngx_http_casual_init.\n");

   return i;
}

static void *
ngx_http_casual_create_loc_conf(ngx_conf_t *cf)
{
   ngx_http_casual_loc_conf_t *conf;
   ngx_write_stderr("ngx_http_casual_create_loc_conf.\n");

   conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_casual_loc_conf_t));
   conf->enable = NGX_CONF_UNSET;
   return conf;
}

static char *
ngx_http_casual_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
   //ngx_http_casual_loc_conf_t *prev = parent;
   ngx_http_casual_loc_conf_t *conf = child;
   ngx_write_stderr("ngx_http_casual_merge_loc_conf.\n");

   if (conf->enable)
      ngx_http_casual_init(conf);

   return NGX_CONF_OK ;
}

char* handleError(ngx_http_request_t *r)
{
   long size = strlen(tperrnostring(tperrno));
   char* error = ngx_pcalloc(r->pool, size);
   strncpy(error, tperrnostring(tperrno), size);
   ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, error);
   r->headers_out.status = NGX_HTTP_BAD_REQUEST;
   return error;
}
