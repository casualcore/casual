//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once



#include "common/log/line.h"
#include "common/log/trace.h"


namespace casual
{
   namespace common
   {
      namespace log
      {
         //! Log with category 'casual.common'
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
