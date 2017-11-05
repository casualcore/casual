//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_BROKER_INCLUDE_BROKER_COMMON_H_
#define CASUAL_MIDDLEWARE_BROKER_INCLUDE_BROKER_COMMON_H_

#include "common/log/stream.h"
#include "common/log/trace.h"

namespace casual
{
   namespace service
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

   } // service
} // casual

#endif // CASUAL_MIDDLEWARE_BROKER_INCLUDE_BROKER_COMMON_H_
