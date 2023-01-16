//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/log/stream.h"
#include "common/log/trace.h"

namespace casual
{
   namespace queue
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

      namespace event
      {
         extern common::log::Stream log;  
      } // event

   } // queue
} // casual


