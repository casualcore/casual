//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "domain/manager/state.h"
#include "domain/manager/admin/model.h"

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

            //auto replace( casual::configuration::domain::Manager configuration);

            //! if element(s) _keys_ is found, there will be an update, otherwise the element(s) will be added
            //! @return Tasks that fullfills the 'put'
            std::vector< admin::model::Task> put( State& state, casual::configuration::domain::Manager configuration);

         } // configuration
      } // manager
   } // domain
} // casual


