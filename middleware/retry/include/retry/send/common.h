//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/log/stream.h"
#include "common/log/trace.h"
#include "common/uuid.h"

namespace casual
{
   namespace retry
   {
      namespace send
      {
         //! identification of the instance
         const common::Uuid identification{ "e32ece3e19544ae69aae5a6326a3d1e9"};
         constexpr auto environment = "CASUAL_RETRY_SEND_PROCESS";

         extern common::log::Stream log;

         namespace trace
         {
            extern common::log::Stream log;
         } // trace

         namespace verbose
         {
            extern common::log::Stream log;
         } // verbose

         struct Trace : common::log::Trace
         {
            template< typename T>
            Trace( T&& value) : common::log::Trace( std::forward< T>( value), trace::log) {}
         };
      } // send
      
   } // retry

} // casual




