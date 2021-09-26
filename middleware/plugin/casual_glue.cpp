#include "casual_glue.h"
#include "http/inbound/call.h"

#include <string>
#include <vector>
#include <utility> // std::move
#include <optional>


struct Impl
{   // data storage for single request
    http::inbound::call::Context context;
    struct
    {
        std::vector<char> data;
        // ...
    } input;

    struct
    {
        std::vector<char> data;
        int code;
        std::string content_type;
        // ...
    } output;
};


extern int casual_glue_start(casual_glue_handle_t *handle)
{
    Impl *impl = new Impl;
    handle->impl = impl;

    return 0;
}

extern int casual_glue_push_chunk(casual_glue_handle_t *handle, const unsigned char *ptr, size_t size)
{
    Impl *impl = reinterpret_cast<Impl*>(handle->impl);
    impl->input.data.insert( std::end( impl->input.data), ptr, ptr + size);
    return 0;
}

extern int casual_glue_call(casual_glue_handle_t *handle, input_t *input)
{
    Impl *impl = reinterpret_cast<Impl*>(handle->impl);

    http::inbound::call::Request request;
    request.service = input->service;
    request.method = input->method;
    request.url = input->url;
    // TODO: headers
    // request.payload.header.emplace_back( "content-length:0");

    // move input pushed with casual_glue_push_chunk
    request.payload.body = std::move(impl->input.data);

    impl->context = http::inbound::call::Context{ http::inbound::call::Directive::service, std::move( request)};
    // return fd to caller
    handle->fd = impl->context.descriptor(); // fd triggered when it's time to poll (when "readable")

    return 0;
}

extern int casual_glue_poll(casual_glue_handle_t *handle, output_t *output)
{
    Impl *impl = reinterpret_cast<Impl*>(handle->impl);

    if (auto reply = impl->context.receive()) 
    {
        impl->output.data.insert( std::begin( impl->output.data), std::begin( reply->payload.body), std::end( reply->payload.body));
        
        // TODO: reply->payload.headers;

        output->status = impl->output.code = reply->code;
        output->data = reinterpret_cast<unsigned char*>(impl->output.data.data());
        output->size = impl->output.data.size();

        return 0;
    }
    else
    {
        return -2; // call again
    }
}

extern int casual_glue_end(casual_glue_handle_t *handle)
{
    Impl *impl = reinterpret_cast<Impl*>(handle->impl);
    delete impl;
    return 0;
}
