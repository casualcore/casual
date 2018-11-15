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
#include "common/communication/ipc.h"
#include "common/message/handle.h"


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
                        state::pending::Request payload( buffer::Payload&& payload)
                        {
                           Trace trace{ "http::outbound::request::local::send::transcode::payload"};

                           state::pending::Request result;

                           auto transcode_base64 = [&]( common::buffer::Payload&& payload)
                           {
                              Trace trace{ "http::outbound::request::local::send::transcode::payload transcode_base64"};

                              platform::binary::type buffer;
                              std::swap( buffer, payload.memory);

                              common::transcode::base64::encode( buffer, payload.memory);

                              return std::move( payload);
                           };

                           auto transcode_none = [&]( common::buffer::Payload&& payload)
                           {
                              Trace trace{ "http::outbound::request::local::send::transcode::payload transcode_none"};
                              return std::move( payload);
                           };

                           static const auto mapping = std::map< std::string, std::function< buffer::Payload( buffer::Payload&&)>>
                           {
                              {
                                 "CFIELD/",
                                 transcode_base64
                              },
                              {
                                 common::buffer::type::binary(),
                                 transcode_base64
                              },
                              {
                                 common::buffer::type::x_octet(),
                                 transcode_base64
                              },
                              {
                                 common::buffer::type::json(),
                                 transcode_none
                              },
                              {
                                 common::buffer::type::xml(),
                                 transcode_none
                              }
                           };

                           auto found = common::algorithm::find( mapping, payload.type);

                           if( found)
                           {
                              log::line( verbose::log, "found transcoder for: ", found->first);
                              result.state().payload = found->second( std::move( payload));
                           }
                           else
                           {
                              log::line( common::log::category::warning, "failed to find a transcoder for buffertype: ", payload.type);
                              log::line( verbose::log, "payload: ", payload);
                              result.state().payload = transcode_none( std::move( payload));
                           }

                           // add content header
                           {
                              auto content = protocol::convert::from::buffer( result.state().payload.type);

                              log::line( verbose::log, "content: ", content);

                              if( ! content.empty())
                              {
                                 result.state().header.request.add( "content-type: " + content);
                              }
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
                           {
                              return write_payload( data, size * nmemb, *state);
                           }

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

            namespace detail
            {
               namespace receive
               {
                  namespace transcode
                  {
                     state::pending::Request payload( state::pending::Request&& request)
                     {
                        Trace trace{ "http::outbound::request::detail::receive::transcode::payload"};

                        log::line( verbose::log, "request from wire: ", request);


                        auto& payload = request.state().payload;

                        // set buffer type
                        {
                           auto content = request.state().header.reply.find( "content-type");

                           if( content)
                           {
                              auto type = protocol::convert::to::buffer( content.value());

                              if( ! type.empty())
                              {
                                 payload.type = std::move( type);
                              }
                              else
                              {
                                 log::line( common::log::category::warning, "failed to deduce buffer type for content-type: ", content.value(), " - action: use same as call buffer");
                              }
                           }
                        }


                        auto transcode_base64 = []( common::buffer::Payload& payload)
                        {
                           Trace trace{ "http::outbound::request::local::send::transcode::payload transcode_base64"};

                           // make sure we've got null termination on payload...
                           payload.memory.push_back( '\0');

                           auto last = common::transcode::base64::decode( payload.memory, std::begin( payload.memory), std::end( payload.memory));
                           payload.memory.erase( last, std::end( payload.memory));
                        };

                        auto transcode_none = []( common::buffer::Payload& payload)
                        {
                        };

                        static const auto mapping = std::map< std::string, std::function< void( buffer::Payload&)>>
                        {
                           {
                              "CFIELD/",
                              transcode_base64
                           },
                           {
                              common::buffer::type::binary(),
                              transcode_base64
                           },
                           {
                              common::buffer::type::x_octet(),
                              transcode_base64
                           },
                           {
                              common::buffer::type::json(),
                              transcode_none
                           },
                           {
                              common::buffer::type::xml(),
                              transcode_none
                           }
                        };

                        auto found = common::algorithm::find( mapping, payload.type);

                        if( found)
                        {
                           log::line( verbose::log, "found transcoder for: ", found->first);
                           found->second( payload);
                        }
                        else
                        {
                           log::line( common::log::category::warning, "failed to find a transcoder for buffertype: ", payload.type);
                           log::line( verbose::log, "payload: ", payload);
                        }

                        log::line( verbose::log, "request after transcoding: ", request);

                        return std::move( request);
                     }
                  } // transcode
               } // receive

            } // detail


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
               request.state().start = now;

               auto& easy = request.easy();

               curl::easy::set::option( easy, CURLOPT_ERRORBUFFER, curl::error::buffer().data());
               curl::easy::set::option( easy, CURLOPT_URL, node.url.data());
               curl::easy::set::option( easy, CURLOPT_FOLLOWLOCATION, 1L);
               curl::easy::set::option( easy, CURLOPT_FAILONERROR, 1L);
               
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

               log::line( http::verbose::log, "request: ", request);

               return request;
            }
           

            namespace code
            {
               common::message::service::call::Reply::Code transform( const common::service::header::Fields& header, curl::type::code::easy code) noexcept
               {
                  common::message::service::call::Reply::Code result;

                  using common::code::xatmi;
                  using curl::type::code::easy;
                  switch( code)
                  {
                     case easy::CURLE_OK:
                     {
                        // the call went ok from curls point of view, lets check 
                        // from casuals point of view.

                        {
                           auto value = header.find( http::header::name::result::code);
                           if( value)
                              result.result = http::header::value::result::code( *value);
                        }
                     
                        {
                           auto value = header.find( http::header::name::result::user::code);
                           if( value)
                              result.user = http::header::value::result::user::code( *value);
                        }
                        break;
                     }
                     default: 
                        result.result = xatmi::service_error;
                        break;
                     
                  }
                  return result;
               }
            } // code

         } // request
      } // outbound
   } // http
} // casual


