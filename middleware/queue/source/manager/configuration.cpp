//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/manager/configuration.h"

#include "queue/manager/transform.h"

#include "configuration/model/change.h"

#include "common/instance.h"
#include "common/message/event.h"

namespace casual
{
   using namespace common;

   namespace queue::manager::configuration
   {
      namespace local
      {
         namespace
         {

            auto spawn_entity( auto& entity)
            {
               entity.process.pid = common::process::spawn(
                  state::entity::path( entity),
                  {},
                  { instance::variable( instance::Information{ entity.configuration.alias})});
               entity.state = decltype( entity.state())::spawned;
               return entity.process.pid;
            };


            auto exit_handle( auto shared)
            {
               return [ shared]( task::unit::id, const common::message::event::process::Exit& event)
               {
                  algorithm::container::erase( shared->pids, event.state.pid);

                  log::line( verbose::log, "pids left: ", shared->pids);

                  if( shared->pids.empty())
                     return task::unit::Dispatch::done;
                  else 
                     return task::unit::Dispatch::pending;
               };
            }

            auto contains_alias( const auto& configuration) 
            {
               return [ &configuration]( auto& group)
               {
                  return algorithm::contains( configuration, group.configuration.alias);
               };
            }

            void added_groups( State& state, std::vector< casual::configuration::model::queue::Group> groups)
            {
               Trace trace{ "queue::manager::configuration::conform::local::added_groups"};

               struct Shared
               {
                  std::vector< strong::process::id> pids;
               };

               auto shared = std::make_shared< Shared>();

               auto spawn_action = [ &state, shared, configuration = std::move( groups)]( task::unit::id) mutable
               {
                  Trace trace{ "queue::manager::configuration::conform spawn_action"};

                  auto groups = algorithm::transform( configuration, []( auto& group){ return state::Group{ std::move( group)};});
                  shared->pids = algorithm::transform( groups, []( auto& group){ return spawn_entity( group);});

                  algorithm::move( std::move( groups), std::back_inserter( state.groups));
                  if( shared->pids.empty())
                     return task::unit::action::Outcome::abort;
                  return task::unit::action::Outcome::success;
               };

               auto connect_handle = [ shared]( task::unit::id, const queue::ipc::message::group::Connect& message)
               {
                  algorithm::container::erase( shared->pids, message.process.pid);

                  if( shared->pids.empty())
                     return task::unit::Dispatch::done;
                  else 
                     return task::unit::Dispatch::pending;
               };

               log::line( verbose::log, "state.task.coordinator: ", state.task.coordinator);

               state.task.coordinator.then( task::create::unit( 
                  task::create::action( "added_groups", std::move( spawn_action)),
                  std::move( connect_handle),
                  exit_handle( shared)
               ));
            }

            void added_forward_groups( State& state, std::vector< casual::configuration::model::queue::forward::Group> groups)
            {
               Trace trace{ "queue::manager::configuration::conform::local::added_forward_groups"};

               struct Shared
               {
                  std::vector< strong::process::id> pids;
               };

               auto shared = std::make_shared< Shared>();

               auto spawn_action = [ &state, shared, configuration = std::move( groups)]( task::unit::id) mutable
               {
                  Trace trace{ "queue::manager::configuration::conform forward groups spawn_action"};

                  auto groups = algorithm::transform( configuration, []( auto& group){ return state::forward::Group{ std::move( group)};});
                  shared->pids = algorithm::transform( groups, []( auto& group){ return spawn_entity( group);});

                  algorithm::move( std::move( groups), std::back_inserter( state.forward.groups));
                  if( shared->pids.empty())
                     return task::unit::action::Outcome::abort;
                  return task::unit::action::Outcome::success;
               };

               auto connect_handle = [ shared]( task::unit::id, const queue::ipc::message::forward::group::Connect& message)
               {
                  algorithm::container::erase( shared->pids, message.process.pid);

                  if( shared->pids.empty())
                     return task::unit::Dispatch::done;
                  else 
                     return task::unit::Dispatch::pending;
               };

               state.task.coordinator.then( task::create::unit( 
                  task::create::action( "added_forward_groups", std::move( spawn_action)),
                  std::move( connect_handle),
                  exit_handle( shared)
               ));
            }

            void removed_groups( State& state, std::vector< casual::configuration::model::queue::Group> groups)
            {
               Trace trace{ "queue::manager::configuration::conform::local::removed_groups"};

               log::line( verbose::log, "state.task.coordinator: ", state.task.coordinator);
               log::line( verbose::log, "groups: ", groups);
               log::line( verbose::log, "state.groups: ", state.groups);

               struct Shared
               {
                  std::vector< strong::process::id> pids;
               };

               auto shared = std::make_shared< Shared>();

               auto shutdown_action = [ &state, shared, configuration = std::move( groups)]( task::unit::id)
               {
                  Trace trace{ "queue::manager::configuration::conform groups shutdown_action"};

                  auto send_shutdown = [ &state]( auto& entity)
                  { 
                     entity.state = state::entity::Lifetime::shutdown;
                     
                     if( entity.process.ipc)
                        state.multiplex.send( entity.process.ipc, common::message::shutdown::Request{ common::process::handle()});
                     else
                        signal::send( entity.process.pid, common::code::signal::terminate);

                     return entity.process.pid;
                  };

                  shared->pids = algorithm::transform_if( state.groups, send_shutdown, contains_alias( configuration));

                  if( shared->pids.empty())
                     return task::unit::action::Outcome::abort;
                  return task::unit::action::Outcome::success;
               };

               state.task.coordinator.then( task::create::unit( 
                  task::create::action( "removed_groups", std::move( shutdown_action)),
                  exit_handle( shared)
               ));
            }

            void removed_forward_groups( State& state, std::vector< casual::configuration::model::queue::forward::Group> groups)
            {
               Trace trace{ "queue::manager::configuration::conform::local::removed_forward_groups"};

               struct Shared
               {
                  std::vector< strong::process::id> pids;
               };

               auto shared = std::make_shared< Shared>();

               auto shutdown_action = [ &state, shared, configuration = std::move( groups)]( task::unit::id)
               {
                  Trace trace{ "queue::manager::configuration::conform forward groups shutdown_action"};

                  log::line( verbose::log, "configuration: ", configuration);
                  log::line( verbose::log, "state.forward.groups: ", state.forward.groups);

                  auto send_shutdown = [ &state]( auto& entity)
                  { 
                     entity.state = state::entity::Lifetime::shutdown;
                     
                     if( entity.process.ipc)
                        state.multiplex.send( entity.process.ipc, common::message::shutdown::Request{ common::process::handle()});
                     else
                        signal::send( entity.process.pid, common::code::signal::terminate);

                     return entity.process.pid;
                  };

                  shared->pids = algorithm::transform_if( state.forward.groups, send_shutdown, contains_alias( configuration));

                  if( shared->pids.empty())
                     return task::unit::action::Outcome::abort;
                  return task::unit::action::Outcome::success;
               };

               state.task.coordinator.then( task::create::unit( 
                  task::create::action( "removed_forward_groups", std::move( shutdown_action)),
                  exit_handle( shared)
               ));
            }


            void modified_groups( State& state, std::vector< casual::configuration::model::queue::Group> groups)
            {
               Trace trace{ "queue::manager::configuration::conform::local::modified_groups"};

               struct Shared
               {
                  std::vector< strong::process::id> pids;
               };

               auto shared = std::make_shared< Shared>();

               auto update_action = [ &state, shared, configuration = std::move( groups)]( task::unit::id)
               {
                  Trace trace{ "queue::manager::configuration::conform groups shutdown_action"};

                  for( auto& group : configuration)
                  {
                     if( auto found = algorithm::find( state.groups, group.alias))
                     {
                        queue::ipc::message::group::configuration::update::Request request{ common::process::handle()};
                        request.model = group;
                        state.multiplex.send( found->process.ipc, request);
                        shared->pids.push_back( found->process.pid);
                     }
                  }
                  if( shared->pids.empty())
                     return task::unit::action::Outcome::abort;
                  return task::unit::action::Outcome::success;
               };

               auto update_reply_handle = [ shared]( task::unit::id, const queue::ipc::message::group::configuration::update::Reply& message)
               {
                  algorithm::container::erase( shared->pids, message.process.pid);

                  if( shared->pids.empty())
                     return task::unit::Dispatch::done;
                  else 
                     return task::unit::Dispatch::pending;
               };

               state.task.coordinator.then( task::create::unit( 
                  task::create::action( "modified_groups", std::move( update_action)),
                  std::move( update_reply_handle),
                  exit_handle( shared)
               ));
            }

            void modified_forward_groups( State& state, std::vector< casual::configuration::model::queue::forward::Group> groups)
            {
               Trace trace{ "queue::manager::configuration::conform::local::modified_forward_groups"};

               struct Shared
               {
                  std::vector< strong::process::id> pids;
               };

               auto shared = std::make_shared< Shared>();

               auto update_action = [ &state, shared, configuration = std::move( groups)]( task::unit::id)
               {
                  Trace trace{ "queue::manager::configuration::conform forward groups modify action"};

                  for( auto& group : configuration)
                  {
                     if( auto found = algorithm::find( state.groups, group.alias))
                     {
                        queue::ipc::message::forward::group::configuration::update::Request request{ common::process::handle()};
                        request.model = group;
                        state.multiplex.send( found->process.ipc, request);
                        shared->pids.push_back( found->process.pid);
                     }
                  }
                  if( shared->pids.empty())
                     return task::unit::action::Outcome::abort;

                  return task::unit::action::Outcome::success;
               };

               auto update_reply_handle = [ shared]( task::unit::id, const queue::ipc::message::forward::group::configuration::update::Reply& message)
               {
                  Trace trace{ "queue::manager::configuration::conform forward groups modify handle"};

                  algorithm::container::erase( shared->pids, message.process.pid);

                  if( shared->pids.empty())
                     return task::unit::Dispatch::done;
                  else 
                     return task::unit::Dispatch::pending;
               };

               state.task.coordinator.then( task::create::unit( 
                  task::create::action( "modified_forward_groups", std::move( update_action)),
                  std::move( update_reply_handle),
                  exit_handle( shared)
               ));
            }
            
         } // <unnamed>
      } // local

      void conform( State& state, casual::configuration::model::queue::Model current, casual::configuration::model::queue::Model wanted)
      {
          Trace trace{ "queue::manager::configuration::conform"};

          state.note = wanted.note;

          log::line( verbose::log, "state.task.coordinator: ", state.task.coordinator);

          auto group_change = casual::configuration::model::change::concrete::calculate( current.groups, wanted.groups);
          auto forward_change = casual::configuration::model::change::concrete::calculate( current.forward.groups, wanted.forward.groups);

          log::line( verbose::log, "group_change: ", group_change);
          log::line( verbose::log, "forward_change: ", forward_change);

         // remove/shutdown first forwards, then groups
         {
            if( ! std::empty( forward_change.removed))
               local::removed_forward_groups( state, std::move( forward_change.removed));
            if( ! std::empty( group_change.removed))
               local::removed_groups( state, std::move( group_change.removed));
         }

         // added 
         {
            if( ! std::empty( group_change.added))
               local::added_groups( state, std::move( group_change.added));
            if( ! std::empty( forward_change.added))
               local::added_forward_groups( state, std::move( forward_change.added));
         }

         // modified 
         {
            if( ! std::empty( forward_change.modified))
               local::modified_forward_groups( state, std::move( forward_change.modified));
            if( ! std::empty( group_change.modified))
               local::modified_groups( state, std::move( group_change.modified));
         }


         log::line( verbose::log, "state.task.coordinator: ", state.task.coordinator);

      }

      void conform( State& state, casual::configuration::message::update::Request&& message)
      {
         state.group_coordinator = { message.model.domain.groups};

         conform( state, transform::configuration( state), std::move( message.model.queue));

         auto send_reply = [ &state, ipc = message.process.ipc, reply = common::message::reverse::type( message)]( task::unit::id)
         {
            state.multiplex.send( ipc, reply);
            return task::unit::action::Outcome::abort;
         };

         // add a task to send reply when previous tasks are dones
         state.task.coordinator.then( task::create::unit( 
            task::create::action( "send update reply", std::move( send_reply))
         ));

      };
      
   } // queue::manager::configuration
   
} // casual
