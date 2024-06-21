//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "http/common.h"

#include "common/algorithm.h"
#include "common/algorithm/compare.h"
#include "common/algorithm/container.h"
#include "common/buffer/type.h"
#include "common/string/compose.h"
#include "common/log/line.h"

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
               code::xatmi code( std::string_view value)
               {
                  using code::xatmi;
                  static const std::map< std::string_view, xatmi> mapping{
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

                  if( auto found = algorithm::find( mapping, value))
                     return found->second;

                  log::line( log::category::error, "unknown result code: ", value, " - protocol error");
                  return xatmi::protocol;
               }

               std::string_view code( common::code::xatmi code)
               {
                  return common::code::description( code);
               }

               namespace user
               {
                  long code( std::string_view value) 
                  { 
                     if( value.empty()) 
                        return 0;
                     return common::string::from< long>( value);
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
                  std::string find( C& container, const K& key, G&& generic)
                  {
                     auto found = common::algorithm::find( container, key);

                     if( found)
                        return std::string{ found->second};

                     log::line( verbose::log, "failed to find key: ", key, " - using generic buffer type protocol");
                     return generic( key);
                  }

                  namespace buffer
                  {
                     namespace type
                     {
                        constexpr auto fielded() { return casual::buffer::field::buffer::key;}
                        constexpr auto string() { return casual::buffer::string::buffer::key;}
                        constexpr auto null() { return common::buffer::type::null;}
                     } // type
                  } // buffer

                  constexpr std::string_view generic_prefix = "application/casual-generic/";

               } // <unnamed>
            } // local

            namespace from
            {
               std::string buffer( std::string_view buffer)
               {
                  static const std::map< std::string_view, std::string_view> mapping{
                     { common::buffer::type::x_octet, protocol::x_octet},
                     { common::buffer::type::binary, protocol::binary},
                     { common::buffer::type::json, protocol::json},
                     { common::buffer::type::xml, protocol::xml},
                     { local::buffer::type::fielded(), protocol::field},
                     { local::buffer::type::string(), protocol::string},
                     { local::buffer::type::null(), protocol::null},
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
               std::string buffer( std::string_view content)
               {
                  static const std::map< std::string_view, std::string_view> mapping{
                     { protocol::x_octet, common::buffer::type::x_octet},
                     { protocol::binary, common::buffer::type::binary},
                     { protocol::json, common::buffer::type::json},
                     { protocol::xml, common::buffer::type::xml},
                     { protocol::field, local::buffer::type::fielded()},
                     { protocol::string, local::buffer::type::string()},
                     { protocol::null, local::buffer::type::null()},
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

                  auto transcode_clear = []( common::buffer::Payload& payload)
                  {
                     payload.data.clear();
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
                     payload.data.push_back( std::byte{ '\0'});

                     auto view = view::binary::to_string_like( payload.data);

                     auto result = common::transcode::base64::decode( std::string_view{ view.data(), view.size()}, payload.data);
                     algorithm::container::trim( payload.data, result);
                  };

                  static const auto mapping = std::map< std::string_view, common::function< void( common::buffer::Payload&) const>>
                  {
                     { common::buffer::type::json, local::transcode_none},
                     { common::buffer::type::xml, local::transcode_none},
                     
                     { common::buffer::type::binary, decode_base64},
                     { common::buffer::type::x_octet, decode_base64},
                     { protocol::convert::local::buffer::type::fielded(), decode_base64},
                     { protocol::convert::local::buffer::type::string(), decode_base64},
                     { protocol::convert::local::buffer::type::null(), local::transcode_clear},
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
                  log::line( verbose::log, "buffer: ", buffer);
                  
                  auto encode_base64 = []( common::buffer::Payload& payload)
                  {
                     auto buffer = std::exchange( payload.data, {});
                     common::transcode::base64::encode( buffer, payload.data);
                  };

                  static const auto mapping = std::map< std::string_view, common::function< void( common::buffer::Payload&) const>>
                  {
                     { common::buffer::type::json, local::transcode_none},
                     { common::buffer::type::xml, local::transcode_none},

                     { common::buffer::type::binary, encode_base64},
                     { common::buffer::type::x_octet, encode_base64},
                     { protocol::convert::local::buffer::type::fielded(), encode_base64},
                     { protocol::convert::local::buffer::type::string(), encode_base64},
                     { protocol::convert::local::buffer::type::null(), local::transcode_clear},
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


