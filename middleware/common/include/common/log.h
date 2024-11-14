//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/log/line.h"
#include "common/log/trace.h"
#include "common/log/category.h"


namespace casual
{
   namespace common
   {

      struct Trace : common::log::Trace
      {
         template< typename T>
         Trace( T&& value) : common::log::Trace( std::forward< T>( value), log::category::trace) {}
      };


   } // common

} // casual
