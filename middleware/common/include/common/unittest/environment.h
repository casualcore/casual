//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/environment.h"
#include "common/execute.h"

namespace casual
{
   namespace common::unittest::environment
   {
      namespace scoped
      {
         inline auto variable( std::string name, std::string_view value)
         {
            auto old = common::environment::variable::get( name);
            common::environment::variable::set( name, value);

            return execute::scope( [ name = std::move( name), old = std::move( old)]()
            {
               if( old)
                  common::environment::variable::set( name, *old);
               else
                  common::environment::variable::unset( name);
            });
         }
         
      } // scoped
      
   } // common::unittest::environment
} // casual