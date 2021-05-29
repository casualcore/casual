#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

// ----[ MODULE SET-UP ]-------------------------------------------------------

static void* ngx_http_xatmi_create_loc_conf(ngx_conf_t *cf);
static char* ngx_http_xatmi_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static char* ngx_http_xatmi_proxy_pass_setup(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_xatmi_request_handler(ngx_http_request_t *r);
static void ngx_http_xatmi_request_data_handler(ngx_http_request_t *r);
static void timer_handler(ngx_event_t *ev);


// 
// LOCATION CONFIGURATION DATA
// ---------------------------
// Stores data from nginx.conf for this http server location.
// Updates when reloading nginx.conf.
//
typedef struct ngx_http_xatmi_loc_conf
{
    ngx_str_t name;
    int initialized;
} ngx_http_xatmi_loc_conf_t;

//
// MODULE CONFIGURATION CALLBACKS
// ------------------------------
// Configures the callbacks for each directive in nginx.conf.
// Sets the name, the valid locations, and expected arguments.
// When using default callbacks, the address of the loc_conf members must be given.
// The list is terminated with ngx_null_command.
// 
static ngx_command_t
ngx_http_xatmi_commands[] =
{
    {   // directive to set HTTP request handler for end-point location
        ngx_string("casual_pass"), // directive name in nginx.conf
        NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS, // is valid for http location scope without argument
        ngx_http_xatmi_proxy_pass_setup, // directive callback that sets the HTTP request handler
        NGX_HTTP_LOC_CONF_OFFSET, // offset for ngx_http_xatmi_loc_conf_t; used by nginx to calculate data pointer address
        0, // not used; offset to struct member; is passed to callback. offsetof(ngx_http_xatmi_loc_conf_t, <struct member>)
        NULL // post handler
    },
    {   // directive to set server name for location
        ngx_string("plugin_name"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1, // is valid for http location scope with one argument
        ngx_conf_set_str_slot, // sets value of ngx_http_xatmi_loc_conf_t.name if specified
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_xatmi_loc_conf_t, name),
        NULL // post handler
    },
   ngx_null_command
};

//
// MODULE CALLBACKS
// ----------------
// Sets the callbacks for when nginx.conf is processed.
// Only the location scope callbacks are used;
//   - `create` allocates and initializes the location scope data.
//   - `merge` can set location scope data from outer scopes if unset locally in nginx.conf.
//
static ngx_http_module_t
ngx_http_xatmi_module_ctx =
{
    NULL, // preconfiguration
    NULL, // postconfiguration
    NULL, // create main configuration
    NULL, // init main configuration
    NULL, // create server configuration
    NULL, // merge server configuration
    ngx_http_xatmi_create_loc_conf, // allocates and initializes location-scope struct
    ngx_http_xatmi_merge_loc_conf   // sets location-scope struct values from outer scope if left unset in location scope
};

//
// MODULE
// ------
// Struct that plugs into nginx modules list.
// No callbacks are used; they would otherwise be called at startup and shutdown of nginx.
//
ngx_module_t
ngx_http_xatmi_module =
{
    NGX_MODULE_V1,
    &ngx_http_xatmi_module_ctx, // module callbacks
    ngx_http_xatmi_commands,    // module configuration callbacks
    NGX_HTTP_MODULE,             // module type is HTTP
    NULL,        // init master
    NULL,        // init module
    NULL,        // init process
    NULL,        // init thread
    NULL,        // exit thread
    NULL,        // exit process
    NULL,        // exit master
    NGX_MODULE_V1_PADDING
};


static char* plugin_init(ngx_conf_t *cf, ngx_http_xatmi_loc_conf_t *location_conf);
static void plugin_exit(void *conf);


// 
// CONFIGURATION INIT
// ------------------
// Callback for initialization of configuration struct. Allocates and initializes values as "unset".
// 
static void*
ngx_http_xatmi_create_loc_conf(ngx_conf_t *cf)
{
    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "plugin: %s", __FUNCTION__);

    // allocates and initializes location-scope struct
    ngx_http_xatmi_loc_conf_t *location_conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_xatmi_loc_conf_t));
    if (location_conf != NULL)
    {   // initialize as unset if merged with other conf
        
    }
    return location_conf;
}

// 
// CONFIGURATION SETUP
// -------------------
// Callback for merging configuration data coming from different scopes in nginx.conf (main, server, or location).
// If unset in both previous and current scope, the default value is used. Only location configuration scope is used.
//
static char*
ngx_http_xatmi_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "plugin: %s", __FUNCTION__);

    ngx_http_xatmi_loc_conf_t *prev = parent;
    ngx_http_xatmi_loc_conf_t *conf = child;
    ngx_conf_merge_str_value(conf->name, prev->name, /*default*/ "/casual/");

    return NGX_CONF_OK;
}

// 
// PROXY PASS SET-UP
// -----------------
// This callback sets the HTTP request handler that proxies to back-end.
// Gets the HTTP module's location configuration struct to update its handler.
//
static char*
ngx_http_xatmi_proxy_pass_setup(ngx_conf_t *cf, ngx_command_t *cmd, /*ngx_http_xatmi_loc_conf_t*/ void *conf)
{
    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "plugin: %s", __FUNCTION__);

    ngx_http_core_loc_conf_t *http_loc_conf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    http_loc_conf->handler = ngx_http_xatmi_request_handler; // sets HTTP request handler

    ngx_http_xatmi_loc_conf_t *location_conf = conf;
    if (!location_conf->initialized)
    {
        return plugin_init(cf, location_conf);
    }
    return NGX_CONF_OK;
}


// ----[ REQUEST HANDLER IMPLEMENTATION ]-----------------------------------------------

//
// REQUEST CONTEXT DATA
// --------------------
// Stores data for a single client HTTP request.
//
typedef struct ngx_http_xatmi_ctx
{
    ngx_http_request_t *r;
    ngx_chain_t *chain;
    ngx_chain_t *chain_last;
    ssize_t content_length;
   //  ngx_str_t service;
    int response_status;
    ngx_event_t timer;
    int state;
    size_t received;
    int last_buffer;
    ngx_str_t content_type;
    u_char *data;
    int number_of_calls;
    int buffer_count;
} ngx_http_xatmi_ctx_t;

static ngx_int_t process_request_parameters(ngx_http_request_t *r, ngx_http_xatmi_ctx_t *request_context);
static ngx_int_t send_response(ngx_http_request_t *r, ngx_http_xatmi_ctx_t *request_context);
static ngx_int_t plugin_call(ngx_http_xatmi_ctx_t *request_context);
static ngx_int_t plugin_receive(ngx_http_xatmi_ctx_t *request_context);
// static ngx_int_t plugin_cancel(ngx_http_xatmi_ctx_t *request_context);

// 
// REQUEST CLEANUP CALLBACK
// ------------------------
// Callback invoked when the request struct is destroyed 
// 
static void
request_cleanup(void *data)
{
    ngx_http_xatmi_ctx_t *request_context = data;
    ngx_log_error(NGX_LOG_NOTICE, request_context->r->connection->log, 0, "plugin: %s", __FUNCTION__);
}

//
// REQUEST HANDLER
// ---------------
// Entry-point callback for each HTTP request. Only accepts GET and POST.
//
static ngx_int_t
ngx_http_xatmi_request_handler(ngx_http_request_t *r)
{
    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "plugin: %s", __FUNCTION__);
    if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_POST)))
    {
        return NGX_HTTP_NOT_ALLOWED; // or NGX_DECLINED
    }
    ngx_http_xatmi_ctx_t *request_context = ngx_pcalloc(r->pool, sizeof(ngx_http_xatmi_ctx_t)); // NOTE: zero-initialized
    if (request_context == NULL)
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    request_context->r = r;
    ngx_http_set_ctx(r, request_context, ngx_http_xatmi_module); // makes context retrievable from r with ngx_http_get_module_ctx(r, ngx_http_xatmi_module)

    ngx_int_t ret;
    if ((ret = process_request_parameters(r, request_context)) != NGX_OK)
    {
        return ret;
    }

    if ((r->method & NGX_HTTP_GET) && ngx_http_discard_request_body(r) != NGX_OK)
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    ret = ngx_http_read_client_request_body(r, ngx_http_xatmi_request_data_handler); // delegates to body handler callback
    if (ret >= NGX_HTTP_SPECIAL_RESPONSE)
    {
        return ret;
    }
    return NGX_DONE; // doesn't destroy request until ngx_http_finalize_request is called
}

// 
// REQUEST DATA HANDLER
// --------------------
// Receives and buffers each data chunk before proxying a contiguous buffer to back-end.
//
static void
ngx_http_xatmi_request_data_handler(ngx_http_request_t *r)
{
    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "plugin: %s", __FUNCTION__);

    ngx_http_xatmi_ctx_t *request_context = ngx_http_get_module_ctx(r, ngx_http_xatmi_module);

    if (request_context->received)
    {
        ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "plugin: ngx_http_xatmi_request_data_handler called twice");
    }

    int last_buffer = 1; // assume true in case no buffer was received

    {ngx_chain_t *it = r->request_body->bufs;
    if (it == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "plugin: r->request_body->bufs == NULL");
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }
    if (!request_context->chain)
    {
        request_context->chain = r->request_body->bufs;
        request_context->chain_last = r->request_body->bufs;
    }
    else
    {
        request_context->chain_last->next = r->request_body->bufs;
        request_context->chain_last = r->request_body->bufs;
    }
    while ( it->next != NULL)
    {
        it = it->next;
        request_context->chain_last = it;
        request_context->received += ngx_buf_size(it->buf);
        last_buffer = it->buf->last_buf;
        request_context->buffer_count += 1;
    }}

    if (last_buffer)
    {
        if (request_context->buffer_count == 1)
        {
            ngx_buf_t *buf = request_context->chain->buf;
            off_t buffer_size = ngx_buf_size(buf);
            if (buf->in_file || buf->temp_file)
            {
                request_context->data = ngx_palloc(r->pool, request_context->received);
                ssize_t bytes_read = ngx_read_file(buf->file, request_context->data, buffer_size, buf->file_pos);
                if ((size_t)bytes_read != request_context->received)
                {
                    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "plugin: error reading tempfile; ret=%zu", bytes_read);
                    ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
                    return;
                }
            }
            else
            {
                request_context->data = buf->pos;
            }
            ngx_str_t str = {buffer_size, request_context->data};
            ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "plugin: request buffer (single) = [%V]", &str);
        }
        else // copy all buffers to contiguous memory
        {
            request_context->data = ngx_palloc(r->pool, request_context->received);
            u_char *pos = request_context->data;

            for (ngx_chain_t *it = request_context->chain; it != NULL; it = it->next)
            {
                off_t buffer_size = ngx_buf_size(it->buf);
                if (it->buf->in_file || it->buf->temp_file)
                {
                    ssize_t bytes_read = ngx_read_file(it->buf->file, pos, buffer_size, it->buf->file_pos);
                    if (bytes_read != buffer_size)
                    {
                        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "plugin: error reading tempfile; ret=%zu", bytes_read);
                        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
                        return;
                    }
                }
                else
                {
                    ngx_memcpy(pos, it->buf->pos, buffer_size);
                }
                ngx_str_t str = {buffer_size, pos};
                ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "plugin: request buffer = [%V]", &str);
                pos += buffer_size;
            }
        }

        ngx_int_t ret = plugin_call(request_context);
        if (ret == NGX_AGAIN)
        {
            request_context->number_of_calls = 1;
            request_context->timer.data = request_context;
            request_context->timer.handler = timer_handler;
            request_context->timer.log = r->connection->log;
            ngx_add_timer(&request_context->timer, /*msec*/0);
        }
        else if (ret == NGX_OK) // response available immediately (response data is set in buffer chain)
        {
            ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "plugin: plugin_call() => \"response available immediately\"");
            ngx_http_finalize_request(r, send_response(r, request_context));
        }
        else // NGX_ERROR
        {
            ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        }
    }
}

static void
timer_handler(ngx_event_t *ev)
{
    ngx_http_xatmi_ctx_t *request_context = ev->data;
    ngx_http_request_t *r = request_context->r;
    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "plugin: %s", __FUNCTION__);

    ngx_int_t ret = NGX_ERROR;
    if (request_context->state < 3)
    {
        ret = plugin_call(request_context);
        if (ret == NGX_OK)
        {
            ret = plugin_receive(request_context);
        }
    }
    else
    {
        ret = plugin_receive(request_context);
    }

    if (ret == NGX_AGAIN)
    {
        request_context->number_of_calls += 1;
        request_context->timer.data = request_context;
        request_context->timer.handler = timer_handler;
        request_context->timer.log = r->connection->log;
        ngx_add_timer(&request_context->timer, /*msec*/10);
    }
    else if (ret == NGX_ERROR)
    {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
    }
    else
    {
        ngx_http_finalize_request(r, send_response(r, request_context));
    }
}

// 
// SEND RESPONSE
// -------------
// Send HTTP header then HTTP body.
// 
static ngx_int_t
send_response(ngx_http_request_t *r, ngx_http_xatmi_ctx_t *request_context)
{
    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "plugin: %s", __FUNCTION__);

    ngx_pool_cleanup_t *cp = ngx_pool_cleanup_add(r->pool, 0);
    if (NULL == cp)
    {
        request_cleanup(r);
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    cp->handler = request_cleanup; // set request's cleanup callback
    cp->data = request_context; // handler argument

    ngx_buf_t *buf = ngx_create_temp_buf(r->pool, request_context->content_length); // allocates and initializes buffer-struct with allocated memory
    ngx_chain_t *chain = ngx_alloc_chain_link(r->pool);
    if (buf == NULL || request_context->chain == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "plugin: failed to allocate memory for response buffer.");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    else // set up the chain with the response in a single buffer
    {
        ngx_memcpy(buf->pos, request_context->data, request_context->content_length);
        buf->last = buf->pos + request_context->content_length;
        buf->last_in_chain = 1;
        buf->last_buf = 1;
        chain->buf = buf;
        chain->next = NULL;
    }

    r->headers_out.status = request_context->response_status;
    r->headers_out.content_length_n = request_context->content_length;
    if (ngx_http_send_header(r) != NGX_OK)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "plugin: ngx_http_send_header(r) != NGX_OK");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    if (request_context->content_length > 0)
    {
        if (ngx_http_output_filter(r, chain) != NGX_OK)
        {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "plugin: ngx_http_output_filter() != NGX_OK");
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
    }
    else
    {
        ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "plugin: request_context->content_length == %zu", request_context->content_length);
    }
    return r->headers_out.status;
}

// 
// PROCESS REQUEST PARAMATERS
// --------------------------
// Invoked on request to process the request data set by nginx.
//
static ngx_int_t
process_request_parameters(ngx_http_request_t *r, ngx_http_xatmi_ctx_t *request_context)
{
    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "plugin: url = '%V'", &r->unparsed_uri);

    if (r->headers_in.content_type)
    {
        request_context->content_type.data = r->headers_in.content_type->value.data;
        request_context->content_type.len = r->headers_in.content_type->value.len;
    }
    else // no content-type header was sent
    {
        request_context->content_type.data = (u_char*)"";
        request_context->content_type.len = 0;
    }

    return NGX_OK;
}



// ----[ BACK-END INTERFACE CALLS ]-------------------------------------------------------

static char* plugin_init(ngx_conf_t *cf, ngx_http_xatmi_loc_conf_t *location_conf)
{
    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "plugin: %s", __FUNCTION__);
    location_conf->initialized = 1;

    ngx_pool_cleanup_t *cln = ngx_pool_cleanup_add(cf->pool, 0);
    if (cln != NULL)
    {
        cln->handler = plugin_exit;
        cln->data = location_conf;
    }
    else
    {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "ngx_pool_cleanup_add -> NULL");
        plugin_exit(location_conf);
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

static void plugin_exit(void *conf)
{
    // ngx_http_xatmi_loc_conf_t *location_conf = conf;

}


static ngx_int_t plugin_call(ngx_http_xatmi_ctx_t *request_context)
{
    if (++request_context->state < 3)
    {
        return NGX_AGAIN;
    }
    else
    {
        return NGX_OK;
    }
}


static ngx_int_t plugin_receive(ngx_http_xatmi_ctx_t *request_context)
{
    if (++request_context->state < 6)
    {
        return NGX_AGAIN;
    }
    else
    {
        ngx_str_t str = {request_context->received, request_context->data};
        ngx_log_error(NGX_LOG_NOTICE, request_context->r->connection->log, 0, "plugin: received = [%V]", &str);
        // you can't send the request data back as response.
        request_context->data = (u_char*)"response";
        request_context->content_length = sizeof "response" - 1;
        request_context->response_status = NGX_HTTP_OK;
        return NGX_OK;
    }
}

// static ngx_int_t plugin_cancel(ngx_http_xatmi_ctx_t *request_context)
// {

// }
