//!
//! casual
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

   } // http
} // casual


#endif /* MIDDLEWARE_HTTP_INCLUDE_HTTP_COMMON_H_ */
