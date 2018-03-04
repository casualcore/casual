//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_CONFIGURATION_EXAMPLE_INCLUDE_MAKER_H_
#define CASUAL_MIDDLEWARE_CONFIGURATION_EXAMPLE_INCLUDE_MAKER_H_

#include "configuration/domain.h"

namespace casual
{
   namespace configuration
   {
      namespace example
      {
         configuration::domain::Manager domain();

         void write( const configuration::domain::Manager& domain, const std::string& file);

         common::file::scoped::Path temporary( const configuration::domain::Manager& domain, const std::string& extension);

      } // example

   } // configuration


} // casual

#endif // CASUAL_MIDDLEWARE_CONFIGURATION_EXAMPLE_INCLUDE_MAKER_H_
