//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_COMMON_H_
#define CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_COMMON_H_

#include "common/log/stream.h"
#include "common/log/trace.h"

namespace casual
{

   namespace domain
   {
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


   } // domain

} // casual

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_COMMON_H_
