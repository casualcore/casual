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
   namespace transaction
   {
      extern common::log::Stream& log; // = common::log::category::transaction;

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

   } // transaction

} // casual


