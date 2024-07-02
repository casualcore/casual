//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "http/inbound/c/api.h"
#include "http/inbound/call.h"
#include "http/common.h"

#include "common/algorithm.h"
#include "common/algorithm/container.h"
#include "common/execution/context.h"
#include "casual/assert.h"


#include <string>
#include <vector>
#include <utility> // std::move
#include <optional>
#include <algorithm>

namespace casual::http::inbound
{
   using namespace casual::common;
   namespace local
   {
      namespace
      {
         namespace header
         {
            // Responsible for memory dealloction for malloc created types
            struct Guard
            {
               Guard() = default;
               ~Guard()
               {
                  Trace trace{ "casual::http::inbound::local::header::~Guard"};

                  std::for_each( headers.data, headers.data + headers.size, []( auto header)
                  {
                     free( header.key);
                     free( header.value);
                  });
                  free( headers.data);
                  headers.data = nullptr;
                  headers.size = 0;
               }
               casual_http_inbound_headers_t headers;
            };
         } // header

         namespace context
         {
            struct Holder
            { // data storage for single request
               casual::http::inbound::call::Context context;
               http::inbound::call::Request request;
               http::inbound::call::Reply reply;
               header::Guard guard;
            };

            context::Holder* cast( void* holder)
            {
               CASUAL_ASSERT( holder != nullptr);
               return static_cast< context::Holder *>( holder);
            }

            void deallocate( casual_http_inbound_handle_t* handle)
            {
               auto context_holder = context::cast( handle->context_holder);
               delete context_holder;
               handle->context_holder = nullptr;
            }

         } // context

         namespace memory
         {
            char* copy( std::string_view value)
            {
               auto result = static_cast< char*>( malloc( value.size() + 1));
               std::copy( std::begin( value), std::end( value), result);
               result[ value.size()] = '\0';
               return result;
            }
         } // memory

         namespace execution::id
         {
            namespace validation
            {
               static const std::regex& format()
               {
                  static const auto format = std::regex{ "[a-f0-9]{32}"};
                  return format;
               }
            }

            void set( const std::vector< http::inbound::call::header::Field>& headers)
            {
               auto found = common::algorithm::find( headers, http::header::name::execution::id);
               if( found && std::regex_match( found->value().data(), validation::format()))
                  common::execution::context::id::set( strong::execution::id{ found->value()});
               else
                  common::execution::context::id::set( strong::execution::id::generate());
            }
         } // execution::id

         void call( casual_http_inbound_handle_t* handle, enum Directive directive)
         {
            Trace trace{ "casual::http::inbound::local::call"};
            auto context_holder = context::cast( handle->context_holder);

            execution::id::set( context_holder->request.payload.header);

            log::line( verbose::log, "request: ", context_holder->request);

            context_holder->context = http::inbound::call::Context{ 
               static_cast< http::inbound::call::Directive>( directive), std::move( context_holder->request)};

            // return fd to caller
            // fd triggered when it's time to poll (when "readable")
            handle->fd = context_holder->context.descriptor().value(); 
         }

         int receive( casual_http_inbound_handle_t* handle)
         {
            Trace trace{ "casual::http::inbound::local::receive"};
            auto context_holder = context::cast( handle->context_holder);

            if( auto reply = context_holder->context.receive())
            {
               // This should not happen
               if( reply->payload.body.empty())
                  reply->payload.body.assign( { std::byte{ 'N'}, std::byte{ 'U'}, std::byte{ 'L'}, std::byte{ 'L'}});

               context_holder->reply = std::move( reply.value());
               return Cycle::done;
            }
            else
               return Cycle::again;
         }

         namespace request
         {
            void set( casual_http_inbound_handle_t* handle, casual_http_inbound_request_t* request)
            {
               Trace trace{ "casual::http::inbound::local::request::set"};
               auto context_holder = context::cast( handle->context_holder);
               context_holder->request.method.assign( request->method.data, request->method.size);
               context_holder->request.url.assign( request->url.data, request->url.size);
               context_holder->request.service.assign( request->service.data, request->service.size);

               // copy all headers i.e. key and value
               std::for_each( request->headers.data, request->headers.data + request->headers.size,[&context_holder]( auto& header)
               {
                  context_holder->request.payload.header.emplace_back( header.key, header.value);
               });
            }
         } // request

         namespace reply
         {
            void get( casual_http_inbound_handle_t* handle, casual_http_inbound_reply_t* reply)
            {
               Trace trace{ "casual::http::inbound::local::reply::get"};

               auto context_holder = context::cast( handle->context_holder);

               auto user_defined_headers = []( auto& header)
               {
                  return ( header.name() != "content-type") && ( header.name() != "content-length");
               };

               const auto header_size = common::algorithm::count_if( context_holder->reply.payload.header, user_defined_headers);

               // use guard to be able to deallocate
               // this is the only dynamic char* list
               context_holder->guard.headers.data = ( casual_http_inbound_header_t *)malloc( header_size * sizeof( casual_http_inbound_header_t));
               context_holder->guard.headers.size = header_size;
               size_t header_number = 0;
               common::algorithm::for_each_if( context_holder->reply.payload.header, [&header_number, &context_holder]( auto header)
               {
                  auto& item = context_holder->guard.headers.data[header_number];
                  item.key = memory::copy( header.name());
                  item.value = memory::copy( header.value());
                  header_number++;
               },
               user_defined_headers
               );

               reply->headers.data = context_holder->guard.headers.data;
               reply->headers.size = context_holder->guard.headers.size;

               if( auto content_type = common::algorithm::find( context_holder->reply.payload.header, "content-type"))
               {
                  // is this safe?              
                  reply->content_type.data = const_cast< char*>( content_type->value().data());
                  reply->content_type.size = content_type->value().size();
               }
               else
               {
                  reply->content_type.data = nullptr;
                  reply->content_type.size = 0;
               }

               reply->code = std::to_underlying( context_holder->reply.code);
               auto span = view::binary::to_string_like( context_holder->reply.payload.body);
               reply->payload.data = span.data();
               reply->payload.size = span.size();
            }
         } // reply

         namespace payload
         {
            void push( casual_http_inbound_handle_t* handle, const unsigned char* data, size_t size)
            {
               Trace trace{ "casual::http::inbound::local::payload::push"};
               if( ! handle->context_holder)
                  handle->context_holder = new context::Holder();
               
               auto context_holder = context::cast( handle->context_holder);

               algorithm::container::append( view::binary::make( data, size), context_holder->request.payload.body);
            }
         } // payload

      }
   }
}

extern void casual_http_inbound_request_set( casual_http_inbound_handle_t* handle, casual_http_inbound_request_t* request)
{
   casual::http::inbound::local::request::set( handle, request);
}

extern void casual_http_inbound_reply_get( casual_http_inbound_handle_t* handle, casual_http_inbound_reply_t* reply)
{
   casual::http::inbound::local::reply::get( handle, reply);
}

extern void casual_http_inbound_push_payload( casual_http_inbound_handle_t* handle, const unsigned char* ptr, size_t size)
{
   casual::http::inbound::local::payload::push( handle, ptr, size);
}

extern void casual_http_inbound_call( casual_http_inbound_handle_t* handle, Directive directive)
{
   casual::http::inbound::local::call( handle, directive);
}

extern int casual_http_inbound_receive( casual_http_inbound_handle_t* handle)
{
   return casual::http::inbound::local::receive( handle);
}

extern void casual_http_inbound_deallocate( casual_http_inbound_handle_t* handle)
{
   return casual::http::inbound::local::context::deallocate( handle);
}