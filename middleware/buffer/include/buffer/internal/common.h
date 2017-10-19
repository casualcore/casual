//!
//! casual
//!

#ifndef MIDDLEWARE_BUFFER_INCLUDE_BUFFER_COMMON_H_
#define MIDDLEWARE_BUFFER_INCLUDE_BUFFER_COMMON_H_

#include "common/log/stream.h"
#include "common/log/trace.h"

namespace casual
{
   namespace buffer
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
   } // buffer
} // casual


#endif /* MIDDLEWARE_BUFFER_INCLUDE_BUFFER_COMMON_H_ */
