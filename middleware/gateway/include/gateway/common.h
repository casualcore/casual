//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_COMMON_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_COMMON_H_

#include "common/log/stream.h"
#include "common/log/trace.h"

namespace casual
{
   namespace gateway
   {
      extern common::log::Stream log;

      struct Trace : common::log::Trace
      {
         template< typename T>
         Trace( T&& value) : common::log::Trace( std::forward< T>( value), log) {}
      };

   } // gateway
} // casual
#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_COMMON_H_
