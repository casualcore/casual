//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "http/inbound/call.h"
#include "http/common.h"

#include "common/message/service.h"
#include "common/communication/ipc.h"
#include "common/code/category.h"

namespace casual
{
   using namespace common;

   namespace http::inbound::call
   { 
      namespace local
      {
         namespace
         {
            namespace buffer
            {
               auto type( const std::vector< header::Field>& header)
               {
                  auto found = algorithm::find( header, "content-type");

                  if( ! found)
                     common::code::raise::error( code::bad_request, "content-type header is manatory");

                  return protocol::convert::to::buffer( found->value);
               }
            } // buffer

            auto send( service::Lookup lookup, Payload payload)
            {
               Trace trace{ "http::inbound::call::local::send"};

               auto& destination = lookup();

               message::service::call::callee::Request message{ process::handle()};
               message.buffer.type = buffer::type( payload.header);
               message.buffer.memory = std::move( payload.body);
               message.header.container() = std::move( payload.header);
               message.pending = destination.pending;
               message.service = destination.service;

               http::buffer::transcode::from::wire( message.buffer);


               return communication::device::blocking::send( destination.process.ipc, message);
            }


            std::optional< Reply> receive( strong::correlation::id& correlation)
            {
               Trace trace{ "http::inbound::call::local::receive"};

               message::service::call::Reply message;

               if( ! communication::device::non::blocking::receive( communication::ipc::inbound::device(), message, correlation))
                  return {};

               log::line( verbose::log, "reply: ", message);

               // reset the correlation so we know that we've consumed the call.
               correlation = {};
               
               http::buffer::transcode::to::wire( message.buffer);

               Reply result;

               result.payload.header.emplace_back( "content-length", std::to_string( message.buffer.memory.size()));
               result.payload.header.emplace_back( "content-type", http::protocol::convert::from::buffer( message.buffer.type));
               result.payload.header.emplace_back( http::header::name::result::code, http::header::value::result::code( message.code.result));
               result.payload.header.emplace_back( http::header::name::result::user::code, http::header::value::result::user::code( message.code.user));

               result.payload.body = std::move( message.buffer.memory);

               auto transform_http_code = []( auto code)
               {
                  switch( code)
                  {
                     case common::code::xatmi::ok: return http::code::ok;
                     case common::code::xatmi::timeout: return http::code::request_timeout;
                     // TODO what to return? can't be a 200. 207?
                     case common::code::xatmi::service_fail: return http::code::internal_server_error;

                     default:
                        return http::code::internal_server_error;
                  }
               };

               result.code = transform_http_code( message.code.result);
               
               return result;
            }
            
         } // <unnamed>
      } // local 

      Context::Context( Arguments arguments)
         : m_payload{ std::move( arguments.payload)}, m_lookup{ std::move( arguments.service)}
      {
         Trace trace{ "http::inbound::call::Context::Context"};
         log::line( verbose::log, "context: ", *this);
      }

      Context::~Context()
      {
         if( m_correlation)
            communication::ipc::inbound::device().discard( m_correlation);
      }

      Context::Context( Context&& other)
         : m_payload{ std::exchange( other.m_payload, {})}, m_lookup{ std::exchange( other.m_lookup, {})}, m_correlation{ std::exchange( other.m_correlation, {})}
      {}

      Context& Context::operator = ( Context&& other)
      {
         m_payload = std::exchange( other.m_payload, {});
         m_lookup = std::exchange( other.m_lookup, {});
         m_correlation = std::exchange( other.m_correlation, {});
         return *this;
      }

      std::optional< Reply> Context::receive() noexcept
      {
         Trace trace{ "http::inbound::call::Context::receive"};

         try
         {
            if( m_correlation)
               return local::receive( m_correlation);

            if( m_lookup && m_lookup.value())
            {
               // reset the lookup optional and move the value into local::call
               m_correlation = local::send( std::move( std::exchange( m_lookup, {}).value()), std::move( m_payload));
            }

            return {};
         }
         catch( ...)
         {
            auto error = exception::capture();

            Reply result;
            result.payload.header.emplace_back( "content-length:0");

            if( common::code::is::category< http::code>( error.code()))
               result.code = static_cast< http::code>( error.code().value());
            if( error.code() == common::code::xatmi::no_entry)
               result.code = http::code::not_found;
            else 
            {
               log::line( log::category::error, error);
               result.code = http::code::internal_server_error;
            }

            return result;
         }
      }


   } // http::inbound::call
} // casual