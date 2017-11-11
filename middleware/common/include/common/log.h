//!
//! casual
//!

#ifndef COMMON_LOG_H
#define COMMON_LOG_H



#include "common/log/stream.h"
#include "common/log/trace.h"


namespace casual
{
   namespace common
   {
      namespace log
      {
         //!
         //! Log with category 'casual.common'
         //!
         extern log::Stream debug;
      } // log

      namespace trace
      {
         extern log::Stream log;
      } // trace

      namespace verbose
      {
         extern log::Stream log;
      } // verbose

      struct Trace : common::log::Trace
      {
         template< typename T>
         Trace( T&& value) : common::log::Trace( std::forward< T>( value), trace::log) {}
      };


   } // common

} // casual



#endif // COMMON_LOG_H
