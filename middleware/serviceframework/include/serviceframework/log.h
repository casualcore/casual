//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/log/stream.h"
#include "common/log/category.h"
#include "common/log/trace.h"

namespace casual
{

   namespace serviceframework
   {
      namespace log
      {
         using common::log::category::parameter;
         extern common::log::Stream debug;
      } // log

      namespace trace
      {
         extern common::log::Stream log;
      } // trace

      struct Trace : common::log::Trace
      {
         template< typename T>
         Trace( T&& value) : common::log::Trace( std::forward< T>( value), trace::log) {}
      };
   } // serviceframework
} // casual


