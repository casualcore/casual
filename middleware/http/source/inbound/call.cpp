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
               //! @returns string pieces (views) owned by the caller
               auto type( const casual::Header& header) -> std::vector< std::string_view>
               {
                  auto content = algorithm::find( header.fields, "content-type");

                  if( auto accept = algorithm::find( header.fields, "accept"))
                  {
                     auto accepts = 
                        accept->value() |
                        std::views::split(',') | 
                        std::views::transform( []( auto type){ return std::string_view{ common::string::trim( type)};});
                     
                     if( content)
                     {
                        if( algorithm::find( accepts, content->value()))
                           return { content->value()};

                        // HTTP behaviour is normally to allow different content-type than what is accepted 
                        // ... but currently, the source and target buffer need to be of the same type
                        return {};
                     }

                     return accepts | std::ranges::to< std::vector>();
                  }

                  if( content)
                     return { content->value()};

                  // HTTP behaviour is normally to assume '*/*' if no content-type is given
                  // ... but currently, the buffer type must be explicit
                  return {};
               }

            } // buffer

            namespace extract::header
            {
               auto trace( casual::Header& header) -> std::tuple< common::strong::execution::id, common::strong::execution::span::id>
               {
                  if( auto field = header.fields.extract( http::header::name::execution::trace::parent))
                     return detail::transform::span( field->value());

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
               auto lookup( const communication::ipc::inbound::Device& ipc, const std::string& service)
               {
                  message::service::lookup::Request lookup{ local::handle( ipc)};
                  lookup.requested = service;
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
                  :  m_ipc{ std::move( ipc)}, 
                     m_request{ Policy::transform( std::move( request))}, 
                     m_correlation{ send::lookup( m_ipc, m_request->service.name)}
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

                     result.buffer.type = call::buffer::type( request.payload.header);
                     result.buffer.data = std::move( request.payload.body);
                     result.buffer.header = std::move( request.payload.header);

                     return result;
                  }

                  static Reply transform( message::service::call::Reply reply)
                  {
                     Reply result;
                     result.payload.body = std::move( reply.buffer.data);
                     result.payload.header.fields.add( { "content-length", std::to_string( result.payload.body.size())});
                     result.payload.header.fields.add( { "content-type", http::protocol::convert::to::content( reply.buffer.type)});
                     result.payload.header.fields.add( { http::header::name::result::code, http::header::value::result::code( reply.code.result)});
                     result.payload.header.fields.add( { http::header::name::result::user::code, http::header::value::result::user::code( reply.code.user)});
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
                     result.payload.header.fields.add( { "content-length", std::to_string( result.payload.body.size())});
                     result.payload.header.fields.add( { "content-type", "text/plain"});
                     result.payload.header.fields.add( { http::header::name::result::code, http::header::value::result::code( static_cast< common::code::xatmi>( error.code().value()))});
                     result.payload.header.fields.add( { http::header::name::result::user::code, http::header::value::result::user::code( 0)});

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
                     result.buffer.header = std::move( request.payload.header);

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

                        return transform::reply::code( reply.code.result);
                     };

                     result.payload.body = std::move( reply.buffer.data);
                     result.payload.header = std::move( reply.buffer.header);
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

      namespace buffer
      {
         auto type( const casual::Header& header) -> std::string_view
         {
            auto source = local::buffer::type( header);
            auto target = source | std::views::transform( []( auto value){ return protocol::convert::to::buffer( value);});
            auto result = std::ranges::find_if( target, [] ( auto value) { return ! value.empty();});
            
            if(result != std::end( target)) 
               return *result;

            common::code::raise::error( code::not_acceptable, "invalid or missing 'content-type'/'accept' headers");
         }
      } // buffer

   } // http::inbound::call
} // casual
