//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "http/inbound/call.h"
#include "http/common.h"

#include "casual/assert.h"
#include "common/message/service.h"
#include "common/communication/ipc.h"
#include "common/communication/instance.h"
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
            auto handle( const communication::ipc::inbound::Device& ipc)
            {
               return process::Handle{ process::id(), ipc.connector().handle().ipc()};
            }
            
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

            namespace transform::reply
            {
               auto code( common::code::xatmi code)
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
            } // transform::reply

            /*

            auto send( service::Lookup lookup, Payload payload)
            {
               Trace trace{ "http::inbound::call::local::send"};

               auto& destination = lookup();

               message::service::call::callee::Request message{ process::handle()};
               message.buffer.type = buffer::type( payload.header);
               message.buffer.data = std::move( payload.body);
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

               result.payload.header.emplace_back( "content-length", std::to_string( message.buffer.data.size()));
               result.payload.header.emplace_back( "content-type", http::protocol::convert::from::buffer( message.buffer.type));
               result.payload.header.emplace_back( http::header::name::result::code, http::header::value::result::code( message.code.result));
               result.payload.header.emplace_back( http::header::name::result::user::code, http::header::value::result::user::code( message.code.user));

               result.payload.body = std::move( message.buffer.data);

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
            */

            namespace send
            {
               auto lookup( const communication::ipc::inbound::Device& ipc, const std::string& service)
               {
                  message::service::lookup::Request lookup{ local::handle( ipc)};
                  lookup.requested = service;
                  lookup.context = decltype( lookup.context.semantic)::no_busy_intermediate;
                  return communication::device::blocking::send( communication::instance::outbound::service::manager::device(), lookup);
               }

               auto request( const communication::ipc::inbound::Device& ipc, message::service::lookup::Reply lookup, message::service::call::callee::Request request)
               {
                  if( lookup.state == decltype( lookup.state)::absent)
                     common::code::raise::error( http::code::not_found, "failed to lookup service: ", lookup.service.name);

                  request.process = local::handle( ipc);
                  request.service = lookup.service;
                  request.pending = lookup.pending;
                  return communication::device::blocking::send( lookup.process.ipc, request);
               }
            } // send
            
            template< typename Policy>
            struct basic_caller
            {
               basic_caller( communication::ipc::inbound::Device ipc, Request request)
                  : correlation{ send::lookup( ipc, request.service)}, ipc{ std::move( ipc)}, request{ Policy::transform( std::move( request))}
               {}

               ~basic_caller()
               {
                  

               }

               basic_caller( basic_caller&&) noexcept = default;
               basic_caller& operator = ( basic_caller&&) noexcept = default;

               std::optional< Reply> operator() ()
               {
                  Trace trace{ "http::inbound::call::local::basic_caller::operator()"};

                  if( request)
                  {
                     // lookup request is in flight
                     message::service::lookup::Reply lookup;
                     if( communication::device::non::blocking::receive( ipc, lookup, correlation))
                        correlation = local::send::request( ipc, std::move( lookup), std::move( std::exchange( request, {}).value()));
                  }
                  else 
                  {
                     // service call is in flight
                     message::service::call::Reply reply;
                     if( communication::device::non::blocking::receive( ipc, reply, correlation))
                        return Policy::transform( std::move( reply));
                  }

                  return {};
               }

               strong::correlation::id correlation;
               communication::ipc::inbound::Device ipc;
               std::optional< message::service::call::callee::Request> request;
            };

            namespace policy
            {
               struct Service
               {
                  static message::service::call::callee::Request transform( Request request)
                  {
                     message::service::call::callee::Request result;
                     result.parent = request.url;
                     result.buffer.type = buffer::type( request.payload.header);
                     result.buffer.data = std::move( request.payload.body);
                     result.header.container() = std::move( request.payload.header);

                     http::buffer::transcode::from::wire( result.buffer);

                     return result;
                  }

                  static Reply transform( message::service::call::Reply reply)
                  {
                     http::buffer::transcode::to::wire( reply.buffer);

                     Reply result;
                     result.payload.body = std::move( reply.buffer.data);
                     result.payload.header.emplace_back( "content-length", std::to_string( result.payload.body.size()));
                     result.payload.header.emplace_back( "content-type", http::protocol::convert::from::buffer( reply.buffer.type));
                     result.payload.header.emplace_back( http::header::name::result::code, http::header::value::result::code( reply.code.result));
                     result.payload.header.emplace_back( http::header::name::result::user::code, http::header::value::result::user::code( reply.code.user));
                     result.code = local::transform::reply::code( reply.code.result);

                     return result;
                  }

               };

               struct Forward
               {
                  static message::service::call::callee::Request transform( Request request)
                  {

                     return {};
                  }
                  
                  static Reply transform( message::service::call::Reply reply)
                  {

                     return {};
                  }

               };


               
            } // policy

            namespace create
            {
               common::unique_function< std::optional< Reply>()> dispatch( communication::ipc::inbound::Device ipc, Directive directive, Request request)
               {
                  Trace trace{ "http::inbound::call::local::create::dispatch"};
                  log::line( verbose::log, "ipc: ", ipc, " directive: ", directive, " request: ", request);

                  switch( directive)
                  {
                     case Directive::forward: return basic_caller< policy::Forward>{ std::move( ipc), std::move( request)};
                     case Directive::service: return basic_caller< policy::Service>{ std::move( ipc), std::move( request)};
                     // possible more flawours of 'http -> service'...
                  }

                  casual::terminate( "invalid value for directive: ", directive);
               }
            } // create
            
         } // <unnamed>
      } // local 

      std::ostream& operator << ( std::ostream& out, Directive value)
      {
         switch( value)
         {
            case Directive::service: return out << "service";
            case Directive::forward: return out << "forward";
         }

         return out << "<unknown>";
      }

      Context::Context( Directive directive, Request request)
      {
         Trace trace{ "http::inbound::call::Context::Context"};

         communication::ipc::inbound::Device ipc;
         m_descriptor = ipc.connector().descriptor();
         m_implementation = local::create::dispatch( std::move( ipc), directive, std::move( request));
      }

      Context::~Context() = default;

      Context::Context( Context&& other)
         : m_descriptor{ std::exchange( other.m_descriptor, {})}, m_implementation{ std::exchange( other.m_implementation, {})}
      {}

      Context& Context::operator = ( Context&& other)
      {
         m_descriptor = std::exchange( other.m_descriptor, {});
         m_implementation = std::exchange( other.m_implementation, {});
         return *this;
      }

      std::optional< Reply> Context::receive() noexcept
      {
         Trace trace{ "http::inbound::call::Context::receive"};

         casual::assertion( m_implementation, common::code::casual::invalid_semantics, " http::inbound::call::Context::receivce");

         try
         {
            auto result = m_implementation();
            if( result)
               m_implementation = {};
            
            return result;
         }
         catch( ...)
         {
            auto error = exception::capture();

            Reply result;
            result.payload.header.emplace_back( "content-length:0");

            if( common::code::is::category< http::code>( error.code()))
               result.code = static_cast< http::code>( error.code().value());
            else if( error.code() == common::code::xatmi::no_entry)
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
