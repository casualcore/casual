//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/environment.h"
#include "common/execute.h"

namespace casual
{
   namespace common::environment::variable::scoped
   {

      //! sets the variable and either set the old value or unsets it on scope exit
      template< typename T>
      auto set( std::string_view name, T&& value)
      {
         auto old = [name]() -> std::tuple< bool, std::string>
         {
            if( variable::exists( name))
               return { true, variable::get( name)};
            else 
               return { false, {}};
         }();

         variable::set( name, std::forward< T>( value));

         return execute::scope( [name, old = std::move( old)]()
         {
            if( std::get< 0>( old))
               variable::set( name, std::get< 1>( old));
            else
               variable::unset( name);
         });
         
      }

   } // common::environment::variable::scoped
} // casual