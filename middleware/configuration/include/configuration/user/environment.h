//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "configuration/user.h"

#include "common/environment.h"

#include <string_view>
#include <vector>


namespace casual
{
   namespace configuration::user::domain::environment
   {

      Environment get( const std::filesystem::path& path);

      std::vector< Variable> fetch( Environment environment);

      std::vector< common::environment::Variable> transform( const std::vector< Variable>& variables);
      std::vector< Variable> transform( const std::vector< common::environment::Variable>& variables);


   } // configuration::user::domain::environment
} // casual




