//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "domain/discovery/admin/model.h"

#include "common/unittest.h"

#include <vector>
#include <string>

namespace casual
{
   namespace domain::unittest::discover
   {
      casual::domain::discovery::admin::model::State state();

      std::vector< std::string> services( std::vector< std::string> services);

      void request( std::vector< std::string> services, std::vector< std::string> queues);

      namespace fetch
      {
         constexpr auto until = common::unittest::fetch::until( &discover::state);

         namespace predicate
         {
            // returns true when `count` providers match the `ability`
            auto provider( message::discovery::api::provider::registration::Ability ability, platform::size::type count) -> common::unique_function< bool( const casual::domain::discovery::admin::model::State&)>;
            
         } // predicate
      } // fetch
      
   } // domain::unittest::
} // casual