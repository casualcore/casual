//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "domain/manager/state.h"
#include "domain/manager/admin/model.h"

#include "configuration/model.h"

namespace casual
{
   namespace domain
   {
      namespace manager
      {
         namespace configuration
         {

            //! extract and transforms the current state to a 'the configuration model'
            casual::configuration::Model get( const State& state);

            //auto replace( casual::configuration::domain::Manager configuration);

            //! if element(s) _keys_ is found, there will be an update, otherwise the element(s) will be added
            //! @return id's of tasks that fullfills the 'put'
            std::vector< common::strong::correlation::id> put( State& state, casual::configuration::Model model);

         } // configuration
      } // manager
   } // domain
} // casual


