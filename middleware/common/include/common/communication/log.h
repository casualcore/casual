//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_LOG_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_LOG_H_

#include "common/log.h"
#include "common/trace.h"

namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace ipc
         {
            //!
            //! Log with category 'casual.ipc'
            //!
            extern common::log::Stream log;

            struct Trace : common::Trace
            {
               template< typename T>
               Trace( T&& value) : common::Trace( std::forward< T>( value), log) {}
            };
         } // ipc

         namespace tcp
         {
            //!
            //! Log with category 'casual.ipc'
            //!
            extern common::log::Stream log;

            struct Trace : common::Trace
            {
               template< typename T>
               Trace( T&& value) : common::Trace( std::forward< T>( value), log) {}
            };
         } // ipc

      } // communication
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_LOG_H_
