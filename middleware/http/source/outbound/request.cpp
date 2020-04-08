//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "http/outbound/request.h"
#include "http/common.h"

#include "common/exception/system.h"
#include "common/string.h"
#include "common/algorithm.h"
#include "common/transcode.h"

#include "common/memory.h"
#include "common/environment.h"
#include "common/communication/ipc.h"
#include "common/message/handle.h"
#include "common/stream.h"


#include <curl/curl.h>

namespace casual
{
   using namespace common;

   namespace http
   {
      namespace outbound
      {     
         namespace request
         {
            namespace local
            {
               namespace
               {
                  namespace send
                  {
                     namespace callback
                     {
                        size_t read_payload( char* buffer, platform::size::type size, state::pending::Request::State& state)
                        {
                           auto source = state.range();

                           log::line( verbose::log, "size: ", size, " - source-size: ", source.size());

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

                     namespace transcode
                     {
                        state::pending::Request payload( common::buffer::Payload&& payload)
                        {
                           Trace trace{ "http::outbound::request::local::send::transcode::payload"};
                           
                           state::pending::Request result;
                           result.state().payload = std::move( payload);

                           http::buffer::transcode::to::wire( result.state().payload);
                           
                           // add content header
                           {
                              auto content = protocol::convert::from::buffer( result.state().payload.type);

                              log::line( verbose::log, "content: ", content);

                              if( ! content.empty())
                                 result.state().header.request.add( "content-type: " + content);
                           }

                           return result;
                        }
                     } // transcode          
                  } // send

                  namespace receive
                  {
                     namespace callback
                     {
                        auto write_payload( char* buffer, platform::size::type size, state::pending::Request::State& state)
                        {
                           Trace trace{ "http::outbound::request::local::receive::callback::write_payload"};
                           
                           auto source = range::make( buffer, size);

                           algorithm::append( source, state.payload.memory);

                           log::line( verbose::log, "wrote ", size, " bytes");

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

                  
                  namespace log
                  {
                     auto verbose = common::environment::variable::get( "CASUAL_CURL_VERBOSE", 0) == 1;
                  } // log
                  
                  namespace global
                  {
                     auto const loggable_content_type_regexp = std::regex{ R"(text\/.*)"};
                  } // global

               } // <unnamed>
            } // local

            namespace receive
            {
               namespace transcode
               {
                  common::buffer::Payload payload( state::pending::Request&& request)
                  {
                     Trace trace{ "http::outbound::request::detail::receive::transcode::payload"};
                     log::line( verbose::log, "request from wire: ", request);

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
                              log::line( common::log::category::error, "failed to deduce buffer type for content-type: ", content.value());

                              if( std::regex_match( content.value(), local::global::loggable_content_type_regexp))
                                 log::line( common::log::category::verbose::error, "payload: ", view::String{ range::make( request.state().payload.memory)});

                              return {};
                           }
                        }
                     }

                     auto payload = std::move( request.state().payload);

                     http::buffer::transcode::from::wire( payload);
                     log::line( verbose::log, "payload: ", payload);

                     return payload;
                  }
               } // transcode
            } // receive
            

            state::pending::Request prepare( const state::Node& node, common::message::service::call::callee::Request&& message)
            {
               Trace trace{ "http::outbound::request::prepare"};

               log::line( http::verbose::log, "node: ", node);

               auto now = platform::time::clock::type::now();

               auto request = local::send::transcode::payload( std::move( message.buffer));

               request.state().header.request.add( *node.headers);

               request.state().destination = message.process;
               request.state().correlation = message.correlation;
               request.state().execution = message.execution;
               request.state().service = std::move( message.service.name);
               request.state().parent = std::move( message.parent);
               request.state().start = now;

               auto& easy = request.easy();

               // clear error buffer
               curl::error::buffer().fill( '\0');
               curl::easy::set::option( easy, CURLOPT_ERRORBUFFER, curl::error::buffer().data());
               
               curl::easy::set::option( easy, CURLOPT_URL, node.url.data());
               curl::easy::set::option( easy, CURLOPT_FOLLOWLOCATION, 1L);

                // connection stuff
               {
                  // TODO performance: there probably exists a better way than to reconnect,
                  //  but in "some" network stacks request gets lost, and no _failure_ is detected. 
                  curl::easy::set::option( easy, CURLOPT_FRESH_CONNECT, 1L);
               }

               
               // always POST? probably...
               curl::easy::set::option( easy, CURLOPT_POST, 1L);


               // prepare the send stuff
               {
                  curl::easy::set::option( easy, CURLOPT_POSTFIELDSIZE_LARGE , request.state().payload.memory.size());
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

               if( local::log::verbose)
                  curl::easy::set::option( easy, CURLOPT_VERBOSE, 1);


               log::line( http::verbose::log, "request: ", request);

               return request;
            }
           

            namespace code
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
               common::message::service::Code transform( const state::pending::Request& request, curl::type::code::easy code) noexcept
               {
                  Trace trace{ "http::outbound::request::code::transform"};

                  auto& header = request.state().header.reply;
                  
                  // check if we've got casual header codes, if so that is the "state", regardless of what curl thinks.

                  if( auto value = header.find( http::header::name::result::code))
                  {
                     common::message::service::Code result;
                     result.result = http::header::value::result::code( *value);
;
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
                     log::line( common::log::category::error, common::code::xatmi::service_error, " curl error: ", curl_easy_strerror( code));
                     log::line( common::log::category::verbose::error, CASUAL_NAMED_VALUE( request));

                     return { common::code::xatmi::service_error, 0};
                  }
               }
            } // code

         } // request
      } // outbound
   } // http
} // casual


