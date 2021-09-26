#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "casual_glue.h"


typedef struct ngx_http_casual_ctx
{
    ngx_http_request_t *r;
    ngx_str_t content_type;
    int response_status;
    ngx_chain_t *output;
    ngx_connection_t dummy_connection;
    ngx_event_t rev;
    ngx_event_t wev;
    ngx_event_t timer;
    casual_glue_handle_t handle;
} ngx_http_casual_ctx_t;

typedef struct ngx_http_casual_main_conf
{   // TODO: main conf is unused for now
    ngx_log_t *log;
} ngx_http_casual_main_conf_t;

typedef struct ngx_http_casual_loc_conf
{
    ngx_str_t directive; // "service" or "http"; TODO: change to enum
} ngx_http_casual_loc_conf_t;

static void* ngx_http_casual_create_main_conf(ngx_conf_t *cf);
static char* ngx_http_casual_init_main_conf(ngx_conf_t *cf, void *main_conf_ptr);
static void* ngx_http_casual_create_loc_conf(ngx_conf_t *cf);
static char* ngx_http_casual_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static char* ngx_http_casual_enable(ngx_conf_t *cf, ngx_command_t *cmd, void *location_conf_ptr);
void empty_handler(ngx_event_t *ev);
static void check_request(ngx_http_casual_ctx_t *casual_request);
static void timer_event_handler(ngx_event_t *timer);
static void read_event_handler(ngx_event_t *rev);
// static void write_event_handler(ngx_event_t *wev);
static ngx_int_t ngx_http_casual_init_process(ngx_cycle_t *cycle);
static void ngx_http_casual_exit_process(ngx_cycle_t *cycle);
static ngx_int_t start_request(ngx_http_casual_ctx_t *casual_request);
static void request_cleanup(void *data);
static ngx_int_t ngx_http_casual_request_handler(ngx_http_request_t *r);
static ngx_int_t process_request_parameters(ngx_http_request_t *r, ngx_http_casual_ctx_t *casual_request);
static void ngx_http_casual_request_data_handler(ngx_http_request_t *r);
static ngx_int_t send_response(ngx_http_casual_ctx_t *casual_request);


static ngx_command_t
ngx_http_casual_commands[] =
{
    {   // directive to set HTTP request handler for location
        ngx_string("casual_enable"),
        NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
        ngx_http_casual_enable,
        NGX_HTTP_LOC_CONF_OFFSET,
        0, // not used;
        NULL // post handler
    },
    {   // "service" or "http"
        ngx_string("casual_directive"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_slot, // TODO: change to enum
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_casual_loc_conf_t, directive),
        NULL // post handler
    },
    ngx_null_command
};

static ngx_http_module_t
ngx_http_casual_module_ctx =
{
    NULL, // preconfiguration
    NULL, // postconfiguration
    ngx_http_casual_create_main_conf, // create main configuration
    ngx_http_casual_init_main_conf, // init main configuration
    NULL, // create server configuration
    NULL, // merge server configuration
    ngx_http_casual_create_loc_conf, // allocates and initializes location-scope struct
    ngx_http_casual_merge_loc_conf   // sets location-scope struct values from outer scope if left unset in location scope
};

ngx_module_t
ngx_http_casual_module =
{
    NGX_MODULE_V1,
    &ngx_http_casual_module_ctx,  // module callbacks
    ngx_http_casual_commands,     // module configuration callbacks
    NGX_HTTP_MODULE,           // module type is HTTP
    NULL,        // init_master
    NULL,        // init_module
    ngx_http_casual_init_process, // init_process
    NULL,        // init_thread
    NULL,        // exit_thread
    ngx_http_casual_exit_process, // exit_process
    NULL,        // exit_master
    NGX_MODULE_V1_PADDING
};

static void*
ngx_http_casual_create_main_conf(ngx_conf_t *cf)
{
    ngx_conf_log_error(NGX_LOG_NOTICE, cf, 0, "casual: %s", __FUNCTION__);
    ngx_http_casual_main_conf_t *main_conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_casual_main_conf_t));
    if (main_conf != NULL)
    {   // unset fields
    }

    return main_conf;
}
static char*
ngx_http_casual_init_main_conf(ngx_conf_t *cf, void *main_conf_ptr)
{
    ngx_conf_log_error(NGX_LOG_NOTICE, cf, 0,"%s", __FUNCTION__);
    // ngx_http_casual_main_conf_t *main_conf = main_conf_ptr;
    return NGX_CONF_OK;
}

static void*
ngx_http_casual_create_loc_conf(ngx_conf_t *cf)
{
    ngx_conf_log_error(NGX_LOG_NOTICE, cf, 0, "casual: %s", __FUNCTION__);
    ngx_http_casual_loc_conf_t *location_conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_casual_loc_conf_t));
    if (location_conf != NULL)
    {   // unset fields
        // location_conf->directive = NGX_CONF_UNSET_UINT;
    }
    return location_conf;
}

static char*
ngx_http_casual_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_conf_log_error(NGX_LOG_NOTICE, cf, 0, "casual: %s", __FUNCTION__);
    ngx_http_casual_loc_conf_t *prev = parent;
    ngx_http_casual_loc_conf_t *location_conf = child;
    ngx_conf_merge_str_value(location_conf->directive, prev->directive, /*default*/ "service"); // TODO: convert to casual Directive
    // ngx_conf_merge_uint_value(location_conf->directive, prev->directive, /*default*/ Directive::service);
    return NGX_CONF_OK;
}

static char*
ngx_http_casual_enable(ngx_conf_t *cf, ngx_command_t *cmd, /*ngx_http_casual_loc_conf_t*/ void *location_conf_ptr)
{
    ngx_conf_log_error(NGX_LOG_NOTICE, cf, 0, "casual: %s", __FUNCTION__);
    ngx_http_core_loc_conf_t *http_loc_conf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    http_loc_conf->handler = ngx_http_casual_request_handler; // sets HTTP request handler
    return NGX_CONF_OK;
}

// --------------------------------------------------------------------------

void
empty_handler(ngx_event_t *ev)
{
    ngx_log_error(NGX_LOG_NOTICE, ev->log, 0, "%s, %s, active = ", __FUNCTION__, ev->write ? "WRITE" : "READ", ev->active);
}

static void
check_request(ngx_http_casual_ctx_t *casual_request)
{
    ngx_log_error(NGX_LOG_NOTICE, casual_request->r->connection->log, 0, "%s", __FUNCTION__);

    output_t output = {0};
    int ret = casual_glue_poll(&casual_request->handle, &output);
    if (ret == 0)
    {
        // remove event handler for read event
        if (casual_request->rev.active)
        {
            casual_request->rev.handler = empty_handler;
            if (NGX_OK != ngx_del_event(&casual_request->rev, NGX_READ_EVENT, NGX_CLOSE_EVENT))
            {
                ngx_log_error(NGX_LOG_ERR, casual_request->r->connection->log, 0, "casual: %s, ngx_del_event(rev, NGX_READ_EVENT, NGX_CLOSE_EVENT)", __FUNCTION__);
            }
        }
        // if (casual_request->wev.active)
        // {
        //     casual_request->wev.handler = empty_handler;
        //     if (NGX_OK != ngx_del_event(&casual_request->wev, NGX_WRITE_EVENT, NGX_CLOSE_EVENT))
        //     {
        //         ngx_log_error(NGX_LOG_ERR, casual_request->r->connection->log, 0, "casual: %s, ngx_del_event(wev, NGX_WRITE_EVENT, NGX_CLOSE_EVENT)", __FUNCTION__);
        //     }
        // }
        if (casual_request->timer.timer_set)
        {
            ngx_del_timer(&casual_request->timer);
        }

        if (output.data && output.size)
        {
            ngx_buf_t *buf = ngx_create_temp_buf(casual_request->r->pool, output.size);
            buf->last = ngx_cpymem(buf->pos, output.data, output.size); // copy casual-received data to buffer
            buf->memory = 1;
            casual_request->output = ngx_alloc_chain_link(casual_request->r->pool);
            casual_request->output->next = NULL;
            casual_request->output->buf = buf;
        }
        casual_request->response_status = output.status;

        // casual_glue_end(&casual_request->handle); // called in cleanup instead (doesn't really need a copy of data now)
        ngx_http_finalize_request(casual_request->r, send_response(casual_request));
    }
}

static void
timer_event_handler(ngx_event_t *timer)
{   // just log every 1 second per pending request
    ngx_http_casual_ctx_t *casual_request = timer->data;
    ngx_log_error(NGX_LOG_NOTICE, casual_request->r->connection->log, 0, "%s", __FUNCTION__);

    ngx_add_timer(&casual_request->timer, 1000 /*ms*/);
}

static void
read_event_handler(ngx_event_t *rev)
{
    ngx_connection_t *connection = rev->data;
    ngx_log_error(NGX_LOG_NOTICE, connection->log, 0, "casual: %s", __FUNCTION__);

    ngx_http_casual_ctx_t *casual_request = connection->data;
    check_request(casual_request);
}

// static void
// write_event_handler(ngx_event_t *wev)
// {
//     ngx_connection_t *connection = wev->data;
//     ngx_log_error(NGX_LOG_NOTICE, connection->log, 0, "casual: %s", __FUNCTION__);

//     ngx_http_casual_ctx_t *casual_request = connection->data;
//     check_request(casual_request);
// }

static ngx_int_t
ngx_http_casual_init_process(ngx_cycle_t *cycle)
{
    ngx_log_error(NGX_LOG_NOTICE, cycle->log, 0, "%s", __FUNCTION__);

    ngx_http_casual_main_conf_t *main_conf = ngx_http_cycle_get_module_main_conf(cycle, ngx_http_casual_module);

    if (!main_conf)
    {
        ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "casual: !main_conf");
        return NGX_ERROR;
    }

    return NGX_OK;
}

static void
ngx_http_casual_exit_process(ngx_cycle_t *cycle)
{
    ngx_log_error(NGX_LOG_WARN, cycle->log, 0, "%s", __FUNCTION__);
    // ngx_http_casual_main_conf_t *main_conf = ngx_http_cycle_get_module_main_conf(cycle, ngx_http_casual_module);
}

static ngx_int_t
start_request(ngx_http_casual_ctx_t *casual_request)
{
    ngx_log_error(NGX_LOG_NOTICE, casual_request->r->connection->log, 0, "casual: %s", __FUNCTION__);

    ngx_http_casual_loc_conf_t *loc_conf = ngx_http_get_module_loc_conf(casual_request->r, ngx_http_casual_module);
    ngx_http_casual_main_conf_t *main_conf = ngx_http_get_module_main_conf(casual_request->r, ngx_http_casual_module);
    if (loc_conf == NULL || main_conf == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, casual_request->r->connection->log, 0, "casual: loc_conf == NULL || main_conf == NULL");
        return NGX_ERROR;
    }

    ngx_pool_cleanup_t *cln = ngx_pool_cleanup_add(casual_request->r->pool, 0);
    if (NULL == cln)
    {
        return NGX_ERROR;
    }
    cln->handler = request_cleanup; // set request's cleanup callback
    cln->data = casual_request; // handler argument

    input_t input = {0};
    // TODO: set parameters
    // input.method =
    // input.url =
    // input.service =
    // input.headers =
    int ret = casual_glue_call(&casual_request->handle, &input); // Context(directive, request)
    if (ret)
    {
        return NGX_ERROR;
    }

    // if response not immediately available
    ngx_connection_t *connection = &casual_request->dummy_connection;
    connection->data = casual_request;
    connection->read = &casual_request->rev;
    connection->write = &casual_request->wev;
    connection->fd = casual_request->handle.fd;
    connection->read->data = connection; // nginx expects ngx_connection_t as event data
    connection->log = casual_request->r->connection->log;
    connection->read->log = casual_request->r->connection->log;
    connection->write->index = NGX_INVALID_INDEX;
    connection->write->data = connection;
    connection->write->write = 1;
    connection->write->log = casual_request->r->connection->log;
    connection->read->index = NGX_INVALID_INDEX;

    connection->read->handler = read_event_handler; // called when fd is readable
    connection->write->handler = empty_handler; // ignore when fd is writable
    if (ngx_handle_read_event(connection->read, 0) != NGX_OK)
    {
        ngx_log_error(NGX_LOG_ERR, casual_request->r->connection->log, 0, "casual: %s, %s", __FUNCTION__, "ngx_handle_write_event/ngx_handle_read_event");
        return NGX_ERROR;
    }

    // connection->write->handler = write_event_handler; // called when fd is writeable
    // connection->read->handler = empty_handler; // ignore when fd is readable
    // if (ngx_handle_write_event(connection->write, 0) != NGX_OK)
    // {
    //     ngx_log_error(NGX_LOG_ERR, casual_request->r->connection->log, 0, "casual: %s, %s", __FUNCTION__, "ngx_handle_write_event/ngx_handle_read_event");
    //     return NGX_ERROR;
    // }

    casual_request->timer.handler = timer_event_handler;
    casual_request->timer.data = casual_request;
    casual_request->timer.log = casual_request->r->connection->log; // timer event needs log
    ngx_add_timer(&casual_request->timer, 1000 /*ms*/);

    return NGX_AGAIN;
}

static void
request_cleanup(void *data)
{
    ngx_http_casual_ctx_t *casual_request = data;
    ngx_log_error(NGX_LOG_NOTICE, casual_request->r->connection->log, 0, "casual: %s", __FUNCTION__);
    // ngx_http_casual_loc_conf_t *loc_conf = ngx_http_get_module_loc_conf(casual_request->r, ngx_http_casual_module);
    // ngx_http_casual_main_conf_t *main_conf = ngx_http_get_module_main_conf(casual_request->r, ngx_http_casual_module);

    if (casual_request->rev.active)
    {
        // casual_request->rev.handler = empty_handler;
        if (NGX_OK != ngx_del_event(&casual_request->rev, NGX_READ_EVENT, NGX_CLOSE_EVENT))
        {
            ngx_log_error(NGX_LOG_ERR, casual_request->r->connection->log, 0, "casual: %s, ngx_del_event(rev, NGX_READ_EVENT, NGX_CLOSE_EVENT)", __FUNCTION__);
        }
    }
    if (casual_request->wev.active)
    {
        // casual_request->wev.handler = empty_handler;
        if (NGX_OK != ngx_del_event(&casual_request->wev, NGX_WRITE_EVENT, NGX_CLOSE_EVENT))
        {
            ngx_log_error(NGX_LOG_ERR, casual_request->r->connection->log, 0, "casual: %s, ngx_del_event(wev, NGX_WRITE_EVENT, NGX_CLOSE_EVENT)", __FUNCTION__);
        }
    }
    if (casual_request->timer.timer_set)
    {
        ngx_del_timer(&casual_request->timer);
    }

    casual_glue_end(&casual_request->handle); // ~Context();
        // NOTE: could call casual_glue_end when poll returns reply instead.
}

static ngx_int_t
ngx_http_casual_request_handler(ngx_http_request_t *r)
{
    ngx_log_error(NGX_LOG_WARN, r->connection->log, 0, "casual: %s pid=%d", __FUNCTION__, ngx_pid);

    if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_POST))) // TODO: all methods
    {
        return NGX_HTTP_NOT_ALLOWED;
    }
    ngx_http_casual_ctx_t *casual_request = ngx_pcalloc(r->pool, sizeof(ngx_http_casual_ctx_t)); // NOTE: zero-initialized
    if (casual_request == NULL)
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    ngx_http_set_ctx(r, casual_request, ngx_http_casual_module); // makes context retrievable from r with ngx_http_get_module_ctx(r, ngx_http_casual_module)
    casual_request->r = r;

    ngx_int_t ret;
    if ((ret = process_request_parameters(r, casual_request)) != NGX_OK)
    {
        return ret;
    }

    if ((r->method & NGX_HTTP_GET) && ngx_http_discard_request_body(r) != NGX_OK)
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    ret = ngx_http_read_client_request_body(r, ngx_http_casual_request_data_handler); // delegates to body handler callback
    if (ret >= NGX_HTTP_SPECIAL_RESPONSE)
    {
        return ret;
    }
    return NGX_DONE; // doesn't destroy request until ngx_http_finalize_request is called
}

static ngx_int_t
process_request_parameters(ngx_http_request_t *r, ngx_http_casual_ctx_t *casual_request)
{
    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "casual: url = '%V'", &r->unparsed_uri);

    // TODO: get more parameters to pass to casual call
    if (r->headers_in.content_type)
    {
        ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "casual: content-type = '%V'", &r->headers_in.content_type->value);
        casual_request->content_type.data = r->headers_in.content_type->value.data;
        casual_request->content_type.len = r->headers_in.content_type->value.len;
    }

    if (r->headers_in.content_length_n)
    {
        ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "casual: content-length = '%O'", r->headers_in.content_length_n);
    }

    return NGX_OK;
}

static void
ngx_http_casual_request_data_handler(ngx_http_request_t *r)
{
    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "casual: %s", __FUNCTION__);

    ngx_http_casual_ctx_t *casual_request = ngx_http_get_module_ctx(r, ngx_http_casual_module);

    int last_buffer = 1;
    for (ngx_chain_t *cl = r->request_body->bufs; cl; cl = cl->next)
    {
        off_t buffer_size = ngx_buf_size(cl->buf);

        if (cl->buf->in_file || cl->buf->temp_file) // if buffered in file, then read entire file into a buffer
        {
            ngx_log_error(NGX_LOG_NOTICE, casual_request->r->connection->log, 0, "casual: buffer in file");
            ngx_buf_t *buf = ngx_create_temp_buf(r->pool, buffer_size);
            ssize_t bytes_read = ngx_read_file(cl->buf->file, buf->pos, buffer_size, cl->buf->file_pos);
            if (bytes_read != (ssize_t)buffer_size)
            {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "casual: error reading tempfile; ret=%zu", bytes_read);
                ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
                return;
            }
            casual_glue_push_chunk(&casual_request->handle, buf->pos, buffer_size);
        }
        else
        {
            casual_glue_push_chunk(&casual_request->handle, cl->buf->pos, buffer_size);
        }

        last_buffer = cl->buf->last_buf;
    }

    if (last_buffer)
    {   // only starts casual call when all data have been received
        ngx_log_error(NGX_LOG_NOTICE, casual_request->r->connection->log, 0, "casual: last buffer");
        ngx_int_t ret = start_request(casual_request);
        if (ret == NGX_ERROR)
        {
            ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        }
        else if (ret == NGX_OK)
        {   // finalize right away on OK; TODO: enable this case and prevent NULL errors
            ngx_http_finalize_request(r, send_response(casual_request));
        }
        // else NGX_AGAIN to finalize later
    }
    else
    {
        ngx_log_error(NGX_LOG_NOTICE, casual_request->r->connection->log, 0, "casual: not last buffer");
    }
}

static ngx_int_t
send_response(ngx_http_casual_ctx_t *casual_request)
{
    ngx_http_request_t *r = casual_request->r;
    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "casual: %s", __FUNCTION__);

    off_t content_length = 0;
    for (ngx_chain_t *cl = casual_request->output; cl; cl = cl->next)
    {
        content_length += ngx_buf_size(cl->buf);
        if (cl->next == NULL)
        {
            cl->buf->last_in_chain = 1;
            cl->buf->last_buf = 1;
        }
    }

    // TODO: get parameters from output
    r->headers_out.status = casual_request->response_status;
    r->headers_out.content_length_n = content_length;
    r->headers_out.content_type.len = sizeof "text/plain" - 1;
    r->headers_out.content_type.data = ngx_palloc(r->pool, r->headers_out.content_type.len);
    ngx_memcpy(r->headers_out.content_type.data, "text/plain", r->headers_out.content_type.len);

    if (ngx_http_send_header(r) != NGX_OK)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "casual: ngx_http_send_header(r) != NGX_OK");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    if (content_length != 0)
    {
        if (ngx_http_output_filter(r, casual_request->output) != NGX_OK)
        {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "casual: ngx_http_output_filter() != NGX_OK");
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
    }

    return r->headers_out.status;
}
