//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/common.h"
#include "common/string.h"

namespace casual
{
   namespace configuration
   {

      namespace alias
      {
         namespace local
         {
            namespace
            {
               const std::regex expression{ "alias-placeholder-[0-9a-f]{32}"};
            } // <unnamed>
         } // local

         namespace generate
         {
            std::string placeholder()
            {
               return common::string::compose( "alias-placeholder-", common::uuid::make());
            }
            
         } // generate

         namespace is
         {
            bool placeholder( const std::string& alias)
            {
               return std::regex_match( alias, local::expression);
            }

         } // is

      } // alias


      common::log::Stream log{ "casual.configuration"};

      namespace verbose
      {
         common::log::Stream log{ "casual.configuration.verbose"};
      } // verbose

      namespace trace
      {
         common::log::Stream log{ "casual.configuration.trace"};
      } // trace
   } // configuration
} // casual
