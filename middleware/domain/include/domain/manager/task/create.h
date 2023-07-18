//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "domain/manager/state.h"
#include "domain/manager/admin/model.h"

#include "casual/task.h"

namespace casual
{
   namespace domain::manager::task::create
   {  
      namespace event
      {
         //! A helper function to make it easier to send "parent events" correctly
         //! Sends a "parent event" with the 'correlation' and 'description'
         //! An optional 'done' function can be provided that will be triggered within the returned one.
         //! @returns a task that will send the _done event_. Suppose to be added to the result of functions as below
         casual::task::Group parent( State& state, std::string description, common::unique_function< void( State&)> done = {}); 
         
         //! A helper function to make it easier to send "sub events" correctly
         //! Sends a "sub event" with the 'correlation' and 'description'
         //! An optional 'done' function can be provided that will be triggered within the returned one.
         //! @returns a task that will send the _done event_. Suppose to be added to the result of functions as below
         casual::task::Group sub( State& state, std::string description, common::unique_function< void( State&)> done = {});  

      } // event

      namespace scale
      {
         std::vector< casual::task::Group> groups( State& state, std::vector< state::dependency::Group> groups);

         //! @returns a task that when invoked calls `action` to get which group to scale
         casual::task::Group group( State& state, common::unique_function< state::dependency::Group( State&)> action);

         //std::vector< casual::task::Group> groups( State& state, std::vector< state::dependency::Group> groups, std::optional< casual::task::Group> done_event);
      } // scale

      namespace restart
      {
         std::vector< casual::task::Group> groups( State& state, std::vector< state::dependency::Group> groups);
      } // restart

      namespace configuration::managers
      {
         std::vector< casual::task::Group> update( State& state, casual::configuration::Model wanted, const std::vector< common::process::Handle>& destinations);
      } // configuration::managers

   } // domain::manager::task::create
} // casual