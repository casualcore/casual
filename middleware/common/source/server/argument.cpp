//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/server/argument.h"



namespace casual
{
   namespace common
   {
      namespace server
      {
         Arguments::Arguments() = default;

         Arguments::Arguments( Arguments&&) = default;
         Arguments& Arguments::operator = (Arguments&&) = default;

         Arguments::Arguments( std::vector< Service> services)
          : Arguments( std::move( services), {}) {}

         Arguments::Arguments( std::vector< Service> services, std::vector< transaction::resource::Link> resources)
         : services( std::move( services)),resources( std::move( resources)) {}


      } // server
   } // common
} // casual
