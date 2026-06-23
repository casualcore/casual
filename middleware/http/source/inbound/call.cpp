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
                  if( auto found = algorithm::find( header, "content-type"))
                      return protocol::convert::to::buffer( found->value());

                  common::code::raise::error( code::bad_request, "content-type header is mandatory");                 
               }
            } // buffer

            namespace extract::header
            {
               auto trace( std::vector< call::header::Field>& header) -> std::tuple< common::strong::execution::id, common::strong::execution::span::id>
               {
                  if( auto found = algorithm::find( header, http::header::name::execution::trace::parent))
                  {
                     auto field = algorithm::container::extract( header, std::begin( found));

                     return detail::transform::span( field.value());
                  }

                  return {};
               }
               
            } // extract::header



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
               auto lookup( const communication::ipc::inbound::Device& ipc, const message::service::call::callee::Request& request)
               {
                  message::service::lookup::Request lookup{ local::handle( ipc)};
                  lookup.requested = request.service.name;
                  lookup.correlation = request.correlation;
                  lookup.execution = request.execution;
                  lookup.parent = request.parent;
                  return communication::device::blocking::send( communication::instance::outbound::service::manager::device(), lookup);
               }

               auto lookup_discard( const communication::ipc::inbound::Device& ipc, const strong::correlation::id& correlation)
               {
                  message::service::lookup::discard::Request request{ local::handle( ipc)};
                  request.correlation = correlation;
                  request.reply = false;
                  communication::device::blocking::send( communication::instance::outbound::service::manager::device(), request);
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
                        // get stuff from lookup-reply (span, deadline, etc)
                        request.update( lookup); 

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
                  :  m_ipc{ std::move( ipc)}, 
                     m_request{ Policy::transform( std::move( request))}, 
                     m_correlation{ send::lookup( m_ipc, *m_request)}
               {}

               ~basic_caller()
               {
                  // if we still got request, the lookup request is still in flight -> discard it
                  if( m_request)
                     send::lookup_discard( m_ipc, m_correlation);
               }

               basic_caller( basic_caller&& other) noexcept
                  : m_ipc{ std::move( other.m_ipc)},
                     m_request{ std::exchange( other.m_request, {})},
                     m_correlation{ other.m_correlation}
                  {}

               basic_caller& operator = ( basic_caller&& other) noexcept
               {
                  m_ipc = std::move( other.m_ipc);
                  m_request = std::exchange( other.m_request, std::move( m_request));
                  m_correlation = other.m_correlation;
                  return *this;
               }


               std::optional< Reply> operator() ()
               {
                  Trace trace{ "http::inbound::call::local::basic_caller::operator()"};

                  try
                  {

                     if( m_request)
                     {
                        // lookup request is in flight, if we get a reply, we can send the service call request
                        if( auto reply = communication::device::non::blocking::receive< message::service::lookup::Reply>( m_ipc, m_correlation))
                           local::send::request( m_ipc, std::move( *reply), std::exchange( m_request, {}).value());
                     }
                     else 
                     {
                        // service call is in flight
                        if( auto reply = communication::device::non::blocking::receive< message::service::call::Reply>( m_ipc, m_correlation))
                           return Policy::transform( std::move( *reply));
                     }

                     return {};
                  }
                  catch( ...)
                  {
                     return Policy::error(common::exception::capture());

                  }
               }

            private:
               communication::ipc::inbound::Device m_ipc;
               std::optional< message::service::call::callee::Request> m_request;
               strong::correlation::id m_correlation;
            };

            namespace policy
            {
               struct Service
               {
                  static message::service::call::callee::Request transform( Request request)
                  {
                     message::service::call::callee::Request result;
                     result.parent.service = request.url;
                     result.service.name = request.service;

                     // extract execution and span from the traceparent header
                     std::tie( result.execution, result.parent.span) = extract::header::trace( request.payload.header);

                     result.buffer.type = buffer::type( request.payload.header);
                     result.buffer.data = std::move( request.payload.body);
                     result.header = std::move( request.payload.header);

                     return result;
                  }

                  static Reply transform( message::service::call::Reply reply)
                  {
                     Reply result;
                     result.payload.body = std::move( reply.buffer.data);
                     result.payload.header.emplace_back( "content-length", std::to_string( result.payload.body.size()));
                     result.payload.header.emplace_back( "content-type", http::protocol::convert::from::buffer( reply.buffer.type));
                     result.payload.header.emplace_back( http::header::name::result::code, http::header::value::result::code( reply.code.result));
                     result.payload.header.emplace_back( http::header::name::result::user::code, http::header::value::result::user::code( reply.code.user));
                     result.code = local::transform::reply::code( reply.code.result);

                     return result;
                  }

                  static Reply error( const std::system_error& error)
                  {
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

                     auto message = common::binary::span::make( std::string_view{ error.what()});

                     result.payload.body.assign( std::begin( message), std::end( message));
                     result.payload.header.emplace_back( "content-length", std::to_string( result.payload.body.size()));
                     result.payload.header.emplace_back( "content-type", "text/plain");
                     result.payload.header.emplace_back( http::header::name::result::code, http::header::value::result::code( static_cast< common::code::xatmi>( error.code().value())));
                     result.payload.header.emplace_back( http::header::name::result::user::code, http::header::value::result::user::code( 0));

                     return result;
                  }

               };

               struct Forward
               {
                  static message::service::call::callee::Request transform( Request request)
                  {
                     message::service::call::callee::Request result;

                     // extract execution and span from the traceparent header
                     std::tie( result.execution, result.parent.span) = extract::header::trace( request.payload.header);

                     // we parent service to propagate the request line
                     result.parent.service = request.request_line;
                     result.service.name = request.service;

                     // this is forward semantics, so we always set buffer type to http/body
                     result.buffer.type = common::buffer::type::http;
                     result.buffer.data = std::move( request.payload.body);
                     result.header = std::move( request.payload.header);

                     return result;
                  }
                  
                  static Reply transform( message::service::call::Reply reply)
                  {
                     Reply result;

                     auto deduce_status_code = []( const message::service::call::Reply& reply)
                     {
                        // if user is set, assume http code from user
                        if( reply.code.user != 0)
                           return static_cast< http::code>( reply.code.user);

                        return local::transform::reply::code( reply.code.result);
                     };

                     result.payload.body = std::move( reply.buffer.data);
                     result.payload.header = std::move( reply.header).extract();
                     result.code = deduce_status_code( reply);

                     return result;
                  }

                  static Reply error( const std::system_error& error)
                  {
                     return policy::Service::error( error); 
                  }

               };


               
            } // policy

            namespace create
            {
               common::unique_function< std::optional< Reply>()> dispatch( communication::ipc::inbound::Device ipc, Directive directive, Request request)
               {
                  Trace trace{ "http::inbound::call::local::create::dispatch"};
                  log::debug( "ipc: ", ipc, ", directive: ", directive, ", request: ", request);

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

      namespace detail
      {
         namespace transform
         {

            auto span( std::string_view value) -> std::tuple< common::strong::execution::id, common::strong::execution::span::id>
            {
               static const std::regex regex{ R"(^00-[0-9a-f]{32}-[0-9a-f]{16}-[0-9a-f]{2}$)"};
               if( ! std::regex_match( std::begin( value), std::end( value), regex))
                  return {};

               auto result = std::tuple< common::strong::execution::id, common::strong::execution::span::id>{};

               auto& [ execution, span] = result;

               transcode::hex::decode( value.substr( 3, 32), binary::span::fixed::make( execution.underlying().get()));
               transcode::hex::decode( value.substr( 36, 16), span.underlying());

               return result;
            }
         } // transform
         
      } // detail

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

         casual::assertion( m_implementation, common::code::casual::invalid_semantics, " http::inbound::call::Context::receive");

         auto result = m_implementation();
         if( result)
            m_implementation = {};
         
         return result;
      }

   } // http::inbound::call
} // casual
