//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "serviceframework/log.h"


namespace casual
{

   namespace serviceframework
   {
      namespace log
      {
         common::log::Stream sf{ "casual.sf"};

      } // log

      namespace trace
      {
         namespace detail
         {
               Scope::~Scope() = default;

               Scope::Scope( const char* information, std::ostream& log)
                  : common::log::trace::basic::Scope( information, log) {}

         } // detail

      } // trace


   } // serviceframework

} // casual
