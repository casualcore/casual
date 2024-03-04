//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "http/outbound/request.h"
#include "http/common.h"

#include "common/string.h"
#include "common/algorithm.h"
#include "common/transcode.h"
#include "common/log/stream.h"

#include "common/memory.h"
#include "common/environment.h"
#include "common/communication/ipc.h"
#include "common/message/dispatch/handle.h"
#include "common/stream.h"
#include "common/uuid.h"


#include <curl/curl.h>

namespace casual
{
   using namespace common;

   namespace http::outbound::request
   {

      namespace local
      {
         namespace
         {
            struct Configuration
            {
               const bool force_fresh_connect = common::environment::variable::get( "CASUAL_HTTP_CURL_FORCE_FRESH_CONNECT", false);
               const bool force_binary_base64 = common::environment::variable::get( "CASUAL_HTTP_FORCE_BINARY_BASE64", false);
               const bool verbose = common::environment::variable::get( "CASUAL_HTTP_CURL_VERBOSE", false);

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( force_fresh_connect);
                  CASUAL_SERIALIZE( force_binary_base64);
                  CASUAL_SERIALIZE( verbose);
               )
            };

            auto& configuration()
            {
               static Configuration result;
               return result;
            }

            struct
            {
               const std::regex loggable_content{ R"(text\/.*)"};

            } global;

            namespace send
            {
               namespace callback
               {
                  size_t read_payload( char* buffer, platform::size::type size, state::pending::Request::State& state)
                  {
                     auto source = state.range();

                     common::log::line( verbose::log, "size: ", size, " - source-size: ", source.size());

                     if( source.size() <= size)
                     {
                        algorithm::copy( source, buffer);
                        auto count = source.size();

                        // we're done and we clear the buffer so we can use it for the reply
                        state.clear();

                        return count;
                     }
                     else
                     {
                        std::copy( std::begin( source), std::begin( source) + size, buffer);
                        state.offset += size;
                        return size;
                     }
                  }

                  size_t read( char* buffer, size_t size, size_t nitems, state::pending::Request::State* state)
                  {
                     Trace trace{ "http::outbound::request::local::send::callback::read"};

                     return read_payload( buffer, size * nitems, *state);
                  }
               } // callback

               namespace prepare
               {
                  state::pending::Request request( common::buffer::Payload&& payload)
                  {
                     Trace trace{ "http::outbound::request::local::prepare::request"};
                     
                     state::pending::Request result;
                     result.state().payload = std::move( payload);

                     if( local::configuration().force_binary_base64)
                        http::buffer::transcode::to::wire( result.state().payload);
                     
                     // add content header
                     {
                        auto content = protocol::convert::from::buffer( result.state().payload.type);

                        common::log::line( verbose::log, "content: ", content);

                        if( ! content.empty())
                           result.state().header.request.add( "content-type: " + content);
                     }

                     return result;
                  }
               } // prepare
       
            } // send

            namespace receive
            {
               namespace callback
               {
                  auto write_payload( char* buffer, platform::size::type size, state::pending::Request::State& state)
                  {
                     Trace trace{ "http::outbound::request::local::receive::callback::write_payload"};
                     
                     auto source = range::make( buffer, size);

                     algorithm::append( source, state.payload.data);

                     common::log::line( verbose::log, "wrote ", size, " bytes");

                     return size;
                  }

                  size_t write( char* data, size_t size, size_t nmemb, state::pending::Request::State* state)
                  {
                     Trace trace{ "http::outbound::request::local::receive::callback::write"};

                     if( state)
                        return write_payload( data, size * nmemb, *state);

                     return 0;
                  }

                  auto write_header( char* buffer, platform::size::type size, state::pending::Request::State& state)
                  {
                     Trace trace{ "http::outbound::request::local::receive::callback::header"};

                     auto range = range::make( buffer, buffer + size);

                     range = std::get< 0>( common::algorithm::divide_if( range, []( char c){
                        return c == '\n' || c == '\r';
                     }));


                     if( range)
                     {
                        auto split = common::algorithm::split( range, ':');

                        
                        auto to_string = []( auto range){
                           range = common::string::trim( range);
                           return std::string( std::begin( range), std::end( range));
                        };
                        
                        state.header.reply.emplace_back(
                           to_string( std::get< 0>( split)),
                           to_string( std::get< 1>( split)));
                     }
                     // else:
                     // Think this is an "empty header" that we'll be invoked as the "last header"
                     //  

                     return size;
                  }

                  std::size_t header( char* buffer, size_t size, size_t nitems, state::pending::Request::State* state)
                  {
                     if( state)
                        return write_header( buffer, size * nitems, *state);
                     
                     return 0;
                  }



               } // callback
            } // receive

         } // <unnamed>
      } // local

      namespace receive
      {
         common::buffer::Payload payload( state::pending::Request&& request)
         {
            Trace trace{ "http::outbound::request::detail::receive::payload"};
            common::log::line( verbose::log, "request from wire: ", request);

            // set buffer type
            {
               auto content = request.state().header.reply.find( "content-type");

               if( content)
               {
                  auto type = protocol::convert::to::buffer( content.value());

                  if( ! type.empty())
                     request.state().payload.type = std::move( type);
                  else
                  {
                     common::log::line( common::log::category::error, "failed to deduce buffer type for content-type: ", content.value());

                     if( std::regex_match( content.value(), local::global.loggable_content))
                        common::log::line( common::log::category::verbose::error, "payload: ", string::view::make( request.state().payload.data));

                     return {};
                  }
               }
            }

            auto payload = std::move( request.state().payload);

            if( local::configuration().force_binary_base64)
               http::buffer::transcode::from::wire( payload);
            
            common::log::line( verbose::log, "payload: ", payload);

            return payload;
         }
      } // receive
      

      state::pending::Request prepare( const state::Node& node, common::message::service::call::callee::Request&& message)
      {
         Trace trace{ "http::outbound::request::prepare"};

         common::log::line( http::verbose::log, "node: ", node);
         common::log::line( http::verbose::log, "configuration: ", local::configuration());

         auto now = platform::time::clock::type::now();

         auto request = local::send::prepare::request( std::move( message.buffer));
         common::log::line( http::verbose::log, "request: ", request);

         request.state().header.request.add( *node.headers);
         request.state().header.request.add( { { http::header::name::execution::id, common::uuid::string( message.execution.value())}});

         request.state().destination = message.process;
         request.state().correlation = message.correlation;
         request.state().execution = message.execution;
         request.state().service = std::move( message.service.name);
         request.state().parent = std::move( message.parent);
         request.state().trid = message.trid;
         request.state().start = now;

         common::log::line( http::verbose::log, "request.state(): ", request.state());

         auto& easy = request.easy();

         // clear error buffer
         curl::error::buffer().fill( '\0');
         curl::easy::set::option( easy, CURLOPT_ERRORBUFFER, curl::error::buffer().data());
         
         curl::easy::set::option( easy, CURLOPT_URL, node.url.data());
         curl::easy::set::option( easy, CURLOPT_FOLLOWLOCATION, 1L);

         // Only force fresh connection if user wants to...
         if( local::configuration().force_fresh_connect)
         {
            // TODO performance: there probably exists a better way than to reconnect,
            //  but in "some" network stacks request gets lost, and no _failure_ is detected. 
            curl::easy::set::option( easy, CURLOPT_FRESH_CONNECT, 1L);

            // we need to indicate that callee should "close" the connection.
            // one could think that curl would handle this by it self, since it "knows"
            // that we using "CURLOPT_FRESH_CONNECT", but no...
            request.state().header.request.add( "connection: close");
         }


         // always POST? probably...
         curl::easy::set::option( easy, CURLOPT_POST, 1L);


         // prepare the send stuff
         {
            curl::easy::set::option( easy, CURLOPT_POSTFIELDSIZE_LARGE , request.state().payload.data.size());
            curl::easy::set::option( easy, CURLOPT_READFUNCTION, &local::send::callback::read);
            curl::easy::set::option( easy, CURLOPT_READDATA , &request.state());

            // headers
            if( request.state().header.request)
               curl::easy::set::option( easy, CURLOPT_HTTPHEADER, request.state().header.request.native());
         }

         // prepare the receive stuff
         {
            curl::easy::set::option( easy, CURLOPT_WRITEFUNCTION, &local::receive::callback::write);
            curl::easy::set::option( easy, CURLOPT_WRITEDATA, &request.state());

            // headers
            curl::easy::set::option( easy, CURLOPT_HEADERFUNCTION, &local::receive::callback::header);
            curl::easy::set::option( easy, CURLOPT_HEADERDATA, &request.state());
         }

         if( local::configuration().verbose)
            curl::easy::set::option( easy, CURLOPT_VERBOSE, 1);


         common::log::line( http::verbose::log, "request: ", request);

         return request;
      }
      


      namespace transform
      {
         namespace local
         {
            namespace
            {
               namespace xatmi
               {
                  auto code = []( const auto& easy)
                  {
                     auto code = curl::response::code( easy);
                     switch( code)
                     {
                        case 404:
                        case 503: // ?
                        case 501: // ?
                           return common::code::xatmi::no_entry;

                        case 408:
                        case 504: 
                           return common::code::xatmi::timeout;

                        case 429:
                           return common::code::xatmi::limit;

                        case 500:
                           return common::code::xatmi::system;
                     };
                     
                     // if http says ok, and we didn't get casual headers -> protocol error
                     if( code >= 200 && code < 300)
                        return common::code::xatmi::protocol;

                     return common::code::xatmi::system;
                  };
               } // xatmi
            } // <unnamed>
         } // local

         common::message::service::Code code( const state::pending::Request& request, curl::type::code::easy code) noexcept
         {
            Trace trace{ "http::outbound::request::transform::code"};

            auto& header = request.state().header.reply;
            
            // check if we've got casual header codes, if so that is the "state", regardless of what curl thinks.

            if( auto value = header.find( http::header::name::result::code))
            {
               common::message::service::Code result;
               result.result = http::header::value::result::code( *value);

               if( auto value = header.find( http::header::name::result::user::code))
                  result.user = http::header::value::result::user::code( *value);

               return result;
            }
            else if( code == curl::type::code::easy::CURLE_OK)
            {
               return { local::xatmi::code( request.easy()), 0};
            }
            else
            {
               common::log::line( common::log::category::error, common::code::xatmi::service_error, " curl error: ", curl_easy_strerror( code));
               common::log::line( common::log::category::verbose::error, CASUAL_NAMED_VALUE( request));

               return { common::code::xatmi::service_error, 0};
            }
         }

         common::message::service::Transaction transaction( const state::pending::Request& request, common::message::service::Code code) noexcept
         {
            Trace trace{ "http::outbound::request::transform::transaction"};

            if( ! request.state().trid)
               return {};

            auto resolve_state = []( auto code)
            {
               switch( code)
               {
                  using result_enum = decltype( common::message::service::Transaction{}.state);
                  using Enum = decltype( code);
                  
                  case Enum::ok: return result_enum::active;
                  default: return result_enum::rollback;
               }
            };

            return { request.state().trid, resolve_state( code.result)};
         }
      } // transform

   } // http::outbound::request
} // casual


