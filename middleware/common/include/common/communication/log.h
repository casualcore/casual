//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_LOG_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_LOG_H_

#include "common/log/stream.h"
#include "common/log/trace.h"

namespace casual
{
   namespace common
   {
      namespace communication
      {
         //!
         //! Log with category 'casual.communication'
         //!
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

      } // communication
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_LOG_H_
