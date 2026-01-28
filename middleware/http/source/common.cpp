//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "http/common.h"

#include "common/algorithm/compare.h"
#include "common/algorithm/container.h"
#include "common/buffer/type.h"
#include "common/string/compose.h"
#include "common/log/line.h"

#include "casual/buffer/field.h"
#include "casual/buffer/order.h"
#include "casual/buffer/string.h"

namespace casual
{
   using namespace common;

   namespace http
   {

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
                  constexpr std::size_t buffer_type = 0;
                  constexpr std::size_t content_type = 1;

                  template< std::size_t key_index, std::size_t value_index>
                  auto find( const auto& key) -> std::string_view
                  {
                     static const std::array mapping
                     {
                        std::pair{ common::buffer::type::x_octet, protocol::x_octet},
                        std::pair{ common::buffer::type::binary, protocol::binary},
                        std::pair{ common::buffer::type::json, protocol::json},
                        std::pair{ common::buffer::type::yaml, protocol::yaml},
                        std::pair{ common::buffer::type::toml, protocol::toml},
                        std::pair{ common::buffer::type::xml, protocol::xml},
                        std::pair{ casual::buffer::field::key, protocol::field},
                        std::pair{ casual::buffer::order::key, protocol::order},
                        std::pair{ casual::buffer::string::key, protocol::string},
                        std::pair{ common::buffer::type::null, protocol::null},
                     };

                     auto result = std::ranges::find( mapping, key, [] ( const auto& value) -> decltype(auto) { return std::get< key_index>( value);});

                     if( result != std::end( mapping))
                        return std::get< value_index>( *result);
                     else
                        return {};
                  }

               } //
            } // local

            namespace to
            {
               std::string_view buffer( std::string_view content)
               {
                  return local::find< local::content_type, local::buffer_type>( content);
               }

               std::string_view content( std::string_view buffer)
               {
                  return local::find< local::buffer_type, local::content_type>( buffer);
               }
            } // to
         } // convert
      } // protocol

   } // http
} // casual


