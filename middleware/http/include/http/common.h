//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef MIDDLEWARE_HTTP_INCLUDE_HTTP_COMMON_H_
#define MIDDLEWARE_HTTP_INCLUDE_HTTP_COMMON_H_

#include "common/log/trace.h"

namespace casual
{
   namespace http
   {
      extern common::log::Stream log;

      namespace verbose
      {
         extern common::log::Stream log;
      } // verbose

      namespace trace
      {
         extern common::log::Stream log;
      } // trace

      struct Trace : common::log::Trace
      {
         template< typename T>
         Trace( T&& value) : common::log::Trace( std::forward< T>( value), trace::log) {}
      };

      namespace protocol
      {

         const std::string& x_octet();
         const std::string& binary();
         const std::string& json();
         const std::string& xml();
         const std::string& field();


         namespace convert
         {
            namespace from
            {
               std::string buffer( const std::string& buffer);
            } // from

            namespace to
            {
               std::string buffer( const std::string& content);
            } // to
         } // convert

      } //protocol
   } // http
} // casual


#endif /* MIDDLEWARE_HTTP_INCLUDE_HTTP_COMMON_H_ */
