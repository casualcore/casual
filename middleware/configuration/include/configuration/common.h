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
   namespace configuration
   {
      namespace alias
      {
         namespace generate
         {
            //! @returns a unique string used to correlate aliases that shall be generated later
            //! to a more human suitable representation 
            std::string placeholder();
            
         } // generate

         namespace is
         {
            bool placeholder( const std::string& alias);
         } // is
      } // alias


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

   } // configuration
} // casual


