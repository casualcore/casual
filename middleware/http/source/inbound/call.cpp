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
#include "common/environment.h"


namespace casual
{
   using namespace common;

   namespace http::inbound::call
   { 
      namespace local
      {
         namespace
         {
            auto& configuration()
            {
               static struct Configuration
               {
                  const bool force_binary_base64 = environment::variable::get< bool>( "CASUAL_HTTP_FORCE_BINARY_BASE64").value_or( false);
               } result;

               return result;
            }

            auto handle( const communication::ipc::inbound::Device& ipc)
            {
               return process::Handle{ process::id(), ipc.connector().handle().ipc()};
            }
            
            namespace buffer
            {
               auto type( const std::vector< header::Field>& header)
               {
                  if( auto found = algorithm::find( header, "content-type"))
                      return protocol::convert::to::buffer( found->value());

                  common::code::raise::error( code::bad_request, "content-type header is manatory");                 
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

            namespace send
            {
               auto lookup( const communication::ipc::inbound::Device& ipc, const std::string& service)
               {
                  message::service::lookup::Request lookup{ local::handle( ipc)};
                  lookup.requested = service;
                  return communication::device::blocking::send( communication::instance::outbound::service::manager::device(), lookup);
               }

               void request( const communication::ipc::inbound::Device& ipc, message::service::lookup::Reply lookup, message::service::call::callee::Request request)
               {
                  switch( lookup.state)
                  {
                     using Enum = decltype( lookup.state);
                     case Enum::absent:
                        common::code::raise::error( common::code::xatmi::no_entry, "failed to lookup service: ", lookup.service.name);
                     case Enum::timeout:
                        common::code::raise::error( common::code::xatmi::timeout, "timeout during lookup of service: ", lookup.service.name);
                     case Enum::idle:
                     {
                        request.process = local::handle( ipc);
                        request.service = lookup.service;
                        request.pending = lookup.pending;
                        request.correlation = lookup.correlation;

                        communication::device::blocking::send( lookup.process.ipc, request);
                        break;
                     }
                  }
               }
            } // send
            
            template< typename Policy>
            struct basic_caller
            {
               basic_caller( communication::ipc::inbound::Device ipc, Request request)
                  : correlation{ send::lookup( ipc, request.service)}, ipc{ std::move( ipc)}, request{ Policy::transform( std::move( request))}
               {}

               ~basic_caller() = default;

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
                        local::send::request( ipc, std::move( lookup), std::move( std::exchange( request, {}).value()));
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
                     result.parent.service = request.url;
                     result.buffer.type = buffer::type( request.payload.header);
                     result.buffer.data = std::move( request.payload.body);
                     result.header = std::move( request.payload.header);

                     if( local::configuration().force_binary_base64)
                        http::buffer::transcode::from::wire( result.buffer);

                     return result;
                  }

                  static Reply transform( message::service::call::Reply reply)
                  {
                     if( local::configuration().force_binary_base64)
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
         m_protocol = local::buffer::type( request.payload.header);
         m_implementation = local::create::dispatch( std::move( ipc), directive, std::move( request));
      }

      Context::~Context() = default;

      Context::Context( Context&& other)
         : m_descriptor{ std::exchange( other.m_descriptor, {})}, m_protocol{ std::exchange( other.m_protocol, {})}, m_implementation{ std::exchange( other.m_implementation, {})}
      {}

      Context& Context::operator = ( Context&& other)
      {
         m_descriptor = std::exchange( other.m_descriptor, {});
         m_protocol = std::exchange( other.m_protocol, {});
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

            if( common::code::is::category< http::code>( error.code()))
               result.code = static_cast< http::code>( error.code().value());
            else if( error.code() == common::code::xatmi::no_entry)
               result.code = http::code::not_found;
            else 
            {
               log::line( log::category::error, error);
               result.code = http::code::internal_server_error;
            }

            common::buffer::Payload payload{ m_protocol};
            {
               auto view = binary::span::make( error.what(), std::strlen( error.what()));
               payload.data.assign( std::begin( view), std::end( view));
            }

            if( local::configuration().force_binary_base64)
                  http::buffer::transcode::to::wire( payload);
            
            result.payload.body = std::move( payload.data);
            result.payload.header.emplace_back( "content-length", std::to_string( result.payload.body.size()));
            result.payload.header.emplace_back( http::header::name::result::code, http::header::value::result::code( static_cast< common::code::xatmi>( error.code().value())));
            result.payload.header.emplace_back( http::header::name::result::user::code, http::header::value::result::user::code( 0));
 
            return result;
         }
      }

   } // http::inbound::call
} // casual
