//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "http/common.h"

#include "common/algorithm.h"
#include "common/buffer/type.h"

#include "buffer/field.h"

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
                     default: return "unknown";
                  }
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
         const std::string& x_octet() { static const auto name = std::string("application/casual-x-octet"); return name;}
         const std::string& binary() { static const auto name = std::string("application/casual-binary"); return name;}
         const std::string& json() { static const auto name = std::string("application/json"); return name;}
         const std::string& xml() { static const auto name = std::string("application/xml"); return name;}
         const std::string& field() { static const auto name = std::string("application/casual-field"); return name;}

         namespace convert
         {
            namespace local
            {
               namespace
               {
                  template< typename C, typename K>
                  auto find( C& container, const K& key)
                  {
                     auto found = common::algorithm::find( container, key);

                     if( found)
                        return found->second;

                     log::line( verbose::log, "failed to find key: ", key);
                     return decltype( found->second){};
                  }

                  namespace buffer
                  {
                     namespace type
                     {
                        auto fielded() { return common::buffer::type::combine( CASUAL_FIELD, nullptr);}


                     } // type


                  } // buffer

               } // <unnamed>
            } // local
            namespace from
            {
               std::string buffer( const std::string& buffer)
               {
                  static const std::map< std::string, std::string> mapping{
                     { common::buffer::type::x_octet(), protocol::x_octet()},
                     { common::buffer::type::binary(), protocol::binary()},
                     { common::buffer::type::json(), protocol::json()},
                     { common::buffer::type::xml(), protocol::xml()},
                     { local::buffer::type::fielded(), protocol::field()},
                  };
                  return local::find( mapping, buffer);
               }
            } // from

            namespace to
            {
               std::string buffer( const std::string& content)
               {
                  static const std::map< std::string, std::string> mapping{
                     { protocol::x_octet(), common::buffer::type::x_octet()},
                     { protocol::binary(), common::buffer::type::binary()},
                     { protocol::json(), common::buffer::type::json()},
                     { protocol::xml(), common::buffer::type::xml()},
                     { protocol::field(), local::buffer::type::fielded()},
                  };

                  return local::find( mapping, content);
               }
            } // to
         } // convert
      } // protocol
   } // http
} // casual


