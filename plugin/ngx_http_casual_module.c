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

static ngx_int_t ngx_http_casual_handler(ngx_http_request_t *r)
{
   ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0,
         "ngx_http_casual_handler.\n");

   ngx_int_t rc;
   ngx_str_t s;
   ngx_chain_t out;
   u_char* service = 0;
   u_char* p;
   ngx_buf_t *b;

   ngx_http_casual_loc_conf_t *cglcf;
   cglcf = ngx_http_get_module_loc_conf(r, ngx_http_casual_module);

   r->headers_out.status = NGX_HTTP_OK;

   if (ngx_http_arg(r, (u_char*) "service", sizeof("service") - 1, &s) == NGX_OK)
   {
      service = ngx_pnalloc(r->pool, s.len + 1);
      if (service == NULL )
      {
         return NGX_ERROR;
      }
      p = ngx_copy(service, s.data, s.len );
      *p++ = '\0';
      ngx_log_debug(NGX_LOG_DEBUG_ALL, r->connection->log, 0, (char*) service);
   }
   else
   {
      return NGX_ERROR;
   }

   if (!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD)))
   {
      return NGX_HTTP_NOT_ALLOWED;
   }

   rc = ngx_http_discard_request_body(r);

   if (rc != NGX_OK && rc != NGX_AGAIN)
   {
      return rc;
   }

   if (r->headers_in.if_modified_since)
   {
      return NGX_HTTP_NOT_MODIFIED;
   }

   r->headers_out.content_type.len = sizeof("text/plain") - 1;
   r->headers_out.content_type.data = (u_char *) "text/plain";

   //
   // Call Casual
   //
   long buffersize = 0;
   char* error = NULL;
   //char* buffer = tpalloc( "STRING", "", 1024);
   char* buffer = tpalloc("X_OCTET", "YAML", 1024);
   if (!buffer)
   {
      error = handleError(r);
      buffersize = strlen(error);
   }
   else
   {
      int cd1 = 0;

      if ((cd1 = tpacall((char*) service, buffer, 0, 0)) == -1)
      {
         error = handleError(r);
         buffersize = strlen(error);
      }
      else
      {
         if (tpgetrply(&cd1, &buffer, &buffersize, 0) == -1)
         {
            error = handleError(r);
            buffersize = strlen(error);
         }
      }
   }

   r->headers_out.content_length_n = buffersize;

   if (r->method == NGX_HTTP_HEAD)
   {
      rc = ngx_http_send_header(r);

      if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
      {
         return rc;
      }
   }

   b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
   if (b == NULL )
   {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
            "Failed to allocate response buffer.");
      return NGX_HTTP_INTERNAL_SERVER_ERROR;
   }

   out.buf = b;
   out.next = NULL;

   u_char *text = ngx_palloc(r->pool, r->headers_out.content_length_n);
   if (text == NULL )
   {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
            "Failed to allocate memory for circle image.");
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

   b->pos = text;
   b->last = text + r->headers_out.content_length_n;

   b->memory = 1;
   b->last_buf = 1;

   rc = ngx_http_send_header(r);

   if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
   {
      return rc;
   }
   return ngx_http_output_filter(r, &out);
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
