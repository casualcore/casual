//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/log.h"
#include "common/log/trace.h"

namespace casual
{
   namespace common
   {
      namespace mockup
      {
         //! Log with category 'casual.mockup'
         extern common::log::Stream log;

         //! Log with category 'casual.mockup.trace'
         extern common::log::Stream trace;

         struct Trace : common::log::Trace
         {
            template< typename T>
            Trace( T&& value) : common::log::Trace( std::forward< T>( value), trace) {}
         };
      } // mockup
   } // common
} // casual


