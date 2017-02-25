//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MOCKUP_LOG_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MOCKUP_LOG_H_

#include "common/log.h"
#include "common/log/trace.h"

namespace casual
{
   namespace common
   {
      namespace mockup
      {
         //!
         //! Log with category 'casual.mockup'
         //!
         extern common::log::Stream log;

         struct Trace : common::log::Trace
         {
            template< typename T>
            Trace( T&& value) : common::log::Trace( std::forward< T>( value), log) {}
         };
      } // mockup
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MOCKUP_LOG_H_
