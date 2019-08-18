//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "domain/manager/state.h"

#include "configuration/domain.h"

namespace casual
{
   namespace domain
   {
      namespace manager
      {
         struct Settings;
         namespace configuration
         {

            State state( const Settings& settings);

            //! extract and transforms the current state to a 'configuration view'
            casual::configuration::domain::Manager get( const State& state);


         } // configuration
      } // manager

   } // domain

} // casual


