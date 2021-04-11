//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "domain/manager/task.h"
#include "domain/manager/state.h"
#include "domain/manager/admin/model.h"

namespace casual
{
   namespace domain
   {
      namespace manager
      {
         namespace task
         {
            namespace create
            {
               namespace scale
               {
                  manager::Task boot( std::vector< state::dependency::Group> groups, common::strong::correlation::id correlation);
                  manager::Task shutdown( std::vector< state::dependency::Group> groups);

                  manager::Task aliases( std::string description, std::vector< state::dependency::Group> groups);
                  manager::Task aliases( std::vector< state::dependency::Group> groups);
                  
               } // scale

               namespace restart
               {
                  manager::Task aliases( std::vector< state::dependency::Group> groups);
               } // restart

            } // create
         } // task
      } // manager
   } // domain
} // casual