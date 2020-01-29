//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "http/common.h"

#include "common/algorithm.h"
#include "common/buffer/type.h"
#include "common/string.h"

#include "casual/buffer/field.h"
#include "casual/buffer/string.h"

namespace casual
{
   using namespace common;

   namespace http
   {
      log::Stream log{ "casual.http"};

      namespace verbose
      {
         log::Stream log{ "casual.http.verbose"};
      } // verbose

      namespace trace
      {
         log::Stream log{ "casual.http.trace"};
      } // trace


      namespace header
      {
         namespace value
         {
            namespace result
            {
               code::xatmi code( const std::string& value)
               {
                  using code::xatmi;
                  static const std::map< std::string, xatmi> mapping{
                     { "", xatmi::ok},
                     { "OK", xatmi::ok},
                     { "TPEBADDESC" , xatmi::descriptor},
                     { "TPEBLOCK", xatmi::no_message},
                     { "TPEINVAL", xatmi::argument},
                     { "TPELIMIT", xatmi::limit},
                     { "TPENOENT", xatmi::no_entry},
                     { "TPEOS", xatmi::os},
                     { "TPEPROTO", xatmi::protocol},
                     { "TPESVCERR", xatmi::service_error},
                     { "TPESVCFAIL", xatmi::service_fail},
                     { "TPESYSTEM", xatmi::system},
                     { "TPETIME", xatmi::timeout},
                     { "TPETRAN", xatmi::transaction},
                     { "TPGOTSIG", xatmi::signal},
                     { "TPEITYPE", xatmi::buffer_input},
                     { "TPEOTYPE", xatmi::buffer_output},
                     { "TPEEVENT", xatmi::event},
                     { "TPEMATCH", xatmi::service_advertised},
                  };

                  auto found = algorithm::find( mapping, value);

                  if( found)
                     return found->second;

                  log::line( log::category::error, "unknown result code: ", value, " - protocol error");
                  return xatmi::protocol;
               }

               const char* code( common::code::xatmi code)
               {
                  using code::xatmi;
                  switch( code)
                  {
                     case xatmi::ok: return "OK";
                     case xatmi::descriptor: return "TPEBADDESC";
                     case xatmi::no_message: return "TPEBLOCK";
                     case xatmi::argument: return "TPEINVAL";
                     case xatmi::limit: return "TPELIMIT";
                     case xatmi::no_entry: return "TPENOENT";
                     case xatmi::os: return "TPEOS";
                     case xatmi::protocol: return "TPEPROTO";
                     case xatmi::service_error: return "TPESVCERR";
                     case xatmi::service_fail: return "TPESVCFAIL";
                     case xatmi::system: return "TPESYSTEM";
                     case xatmi::timeout: return "TPETIME";
                     case xatmi::transaction: return "TPETRAN";
                     case xatmi::signal: return "TPGOTSIG";
                     case xatmi::buffer_input: return "TPEITYPE";
                     case xatmi::buffer_output: return "TPEOTYPE";
                     case xatmi::event: return "TPEEVENT";
                     case xatmi::service_advertised: return "TPEMATCH";
                  }
                  return "unknown";
               }

               namespace user
               {
                  long code( const std::string& value) 
                  { 
                     if( value.empty()) 
                        return 0;
                     return std::stol( value);
                  }
                  std::string code( long code) { return std::to_string( code);}
                  
               } // user
            } // result            
         } // value
      } // header

      namespace protocol
      {

         namespace convert
         {
            namespace local
            {
               namespace
               {
                  template< typename C, typename K, typename G>
                  auto find( C& container, const K& key, G&& generic)
                  {
                     auto found = common::algorithm::find( container, key);

                     if( found)
                        return found->second;

                     log::line( verbose::log, "failed to find key: ", key, " - using generic buffer type protocol");
                     return generic( key);
                  }

                  namespace buffer
                  {
                     namespace type
                     {
                        auto fielded() { return common::buffer::type::combine( CASUAL_FIELD, nullptr);}
                        auto string() { return common::buffer::type::combine( CASUAL_STRING, nullptr);}
                     } // type
                  } // buffer

                  const std::string generic_prefix{ "application/casual-generic/"};

               } // <unnamed>
            } // local
            namespace from
            {
               std::string buffer( const std::string& buffer)
               {
                  static const std::map< std::string, std::string> mapping{
                     { common::buffer::type::x_octet(), protocol::x_octet},
                     { common::buffer::type::binary(), protocol::binary},
                     { common::buffer::type::json(), protocol::json},
                     { common::buffer::type::xml(), protocol::xml},
                     { local::buffer::type::fielded(), protocol::field},
                     { local::buffer::type::string(), protocol::string},
                  };

                  auto generic = []( auto& buffer)
                  {
                     return common::string::compose( local::generic_prefix, buffer);
                  };

                  return local::find( mapping, buffer, generic);
               }
            } // from

            namespace to
            {
               std::string buffer( const std::string& content)
               {
                  static const std::map< std::string, std::string> mapping{
                     { protocol::x_octet, common::buffer::type::x_octet()},
                     { protocol::binary, common::buffer::type::binary()},
                     { protocol::json, common::buffer::type::json()},
                     { protocol::xml, common::buffer::type::xml()},
                     { protocol::field, local::buffer::type::fielded()},
                     { protocol::string, local::buffer::type::string()},
                  };

                  // tries to deduce the buffer type generic
                  auto generic = []( auto& content) -> std::string
                  {
                     auto prefix = common::algorithm::search( content, local::generic_prefix);

                     if( std::begin( prefix) != std::begin( content))
                     {
                        log::line( common::log::category::error , "failed to deduce casual generic content - content: ", content);
                        return {};
                     }

                     return { std::begin( content) + local::generic_prefix.size(), std::end( content)};
                  };

                  return local::find( mapping, content, generic);
               }
            } // to
         } // convert
      } // protocol

      namespace buffer
      {
         namespace transcode
         {
            namespace local
            {
               namespace
               {
                  auto transcode_none = []( common::buffer::Payload& payload)
                  {
                     // no-op
                  };

               } // <unnamed>
            } // local
            namespace from
            {
               void wire( common::buffer::Payload& buffer)
               {
                  Trace trace{ "http::buffer::transcode::from::wire"};

                  auto decode_base64 = []( common::buffer::Payload& payload)
                  {
                     // make sure we've got null termination on payload...
                     payload.memory.push_back( '\0');

                     auto last = common::transcode::base64::decode( payload.memory, std::begin( payload.memory), std::end( payload.memory));
                     payload.memory.erase( last, std::end( payload.memory));
                  };

                  static const auto mapping = std::map< std::string, std::function< void( common::buffer::Payload&)>>
                  {
                     { common::buffer::type::json(), local::transcode_none},
                     { common::buffer::type::xml(), local::transcode_none},
                     
                     { common::buffer::type::binary(), decode_base64},
                     { common::buffer::type::x_octet(), decode_base64},
                     { protocol::convert::local::buffer::type::fielded(), decode_base64},
                     { protocol::convert::local::buffer::type::string(), decode_base64},
                  };

                  if( auto found = algorithm::find( mapping, buffer.type))
                     found->second( buffer);
                  else
                     // all other buffers we assume is base64 encoded...
                     decode_base64( buffer);

               }

            } // from

            namespace to
            {
               void wire( common::buffer::Payload& buffer)
               {
                  Trace trace{ "http::buffer::transcode::to::wire"};
                  
                  auto encode_base64 = []( common::buffer::Payload& payload)
                  {
                     auto buffer = std::exchange( payload.memory, {});
                     common::transcode::base64::encode( buffer, payload.memory);
                  };

                  static const auto mapping = std::map< std::string, std::function< void( common::buffer::Payload&)>>
                  {
                     { common::buffer::type::json(), local::transcode_none},
                     { common::buffer::type::xml(), local::transcode_none},

                     { common::buffer::type::binary(), encode_base64},
                     { common::buffer::type::x_octet(), encode_base64},
                     { protocol::convert::local::buffer::type::fielded(), encode_base64},
                     { protocol::convert::local::buffer::type::string(), encode_base64},
                  };

                  if( auto found = algorithm::find( mapping, buffer.type))
                     found->second( buffer);
                  else
                     // all other buffers we assume is base64 encoded...
                     encode_base64( buffer);
               }
            } // from 

         } // transcode
      } // buffer

   } // http
} // casual


