//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
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
         namespace ipc
         {
            //!
            //! Log with category 'casual.ipc'
            //!
            extern common::log::Stream log;

            struct Trace : common::log::Trace
            {
               template< typename T>
               Trace( T&& value) : common::log::Trace( std::forward< T>( value), log) {}
            };
         } // ipc

         namespace tcp
         {
            //!
            //! Log with category 'casual.ipc'
            //!
            extern common::log::Stream log;

            struct Trace : common::log::Trace
            {
               template< typename T>
               Trace( T&& value) : common::log::Trace( std::forward< T>( value), log) {}
            };
         } // ipc

      } // communication
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_LOG_H_
