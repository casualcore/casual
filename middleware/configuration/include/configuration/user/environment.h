//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "configuration/user.h"

#include "common/serialize/macro.h"
#include "common/environment.h"

#include <string>
#include <vector>


namespace casual
{
   namespace configuration
   {
      namespace user
      {
         namespace environment
         {

            Environment get( const std::string& file);

            std::vector< Variable> fetch( Environment environment);

            std::vector< common::environment::Variable> transform( const std::vector< Variable>& variables);
            std::vector< Variable> transform( const std::vector< common::environment::Variable>& variables);

         } // environment

      } // user
   } // config
} // casual




