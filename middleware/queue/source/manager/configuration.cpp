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

                  log::debug( "pids left: ", shared->pids);

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

            //! this handles the whole startup of new entities.
            //!  - spawns the entities
            //!  - when connected, sends configuration request
            //!  - when configuration reply is received, marks entity as running
            //!  - when all entities are running (or exited), the task is done
            template< typename ConnectMessage, typename ConfigurationRequest>
            void added_entities( State& state, auto configuration, auto& destination, std::string task_name)
            {
               Trace trace{ "queue::manager::configuration::conform::local::added_entities"};

               struct Shared
               {
                  std::vector< strong::process::id> pids;
               };

               auto shared = std::make_shared< Shared>();

               auto spawn_action = [ &destination, shared, configuration = std::move( configuration)]( task::unit::id) mutable
               {
                  Trace trace{ "queue::manager::configuration::conform spawn_action"};

                  using Entity = std::ranges::range_value_t< decltype( destination)>;

                  auto entities = algorithm::transform( configuration, []( auto& group){ return Entity{ std::move( group)};});
                  shared->pids = algorithm::transform( entities, []( auto& entity){ return spawn_entity( entity);});

                  algorithm::move( std::move( entities), std::back_inserter( destination));

                  if( shared->pids.empty())
                     return task::unit::action::Outcome::abort;
                  else
                     return task::unit::action::Outcome::success;
               };

               auto connect_handle = [ &state, &destination, shared]( task::unit::id, const ConnectMessage& message)
               {
                  Trace trace{ "queue::manager::configuration::conform connect_handle"};

                  auto send_configuration_request = [ &state, &message]( auto& entity)
                  {
                     ConfigurationRequest request{ common::process::handle()};
                     request.model = entity.configuration;

                     state.multiplex.send( message.process.ipc, request);
                  };

                  if( auto found = algorithm::find( destination, message.process.pid))
                  {
                     found->state = decltype( found->state())::connected;
                     found->process = message.process;

                     send_configuration_request( *found);
                  }
                  else
                     log::error( code::casual::invalid_semantics, "failed to correlate entity - ", message.process.pid);

                  return task::unit::Dispatch::pending;
               };

               using ConfigurationReply = common::message::reverse::type_t< ConfigurationRequest>;

               auto configuration_reply = [ &destination, shared]( task::unit::id, const ConfigurationReply& message)
               {
                  Trace trace{ "queue::manager::configuration::conform configuration_reply"};
                  log::debug( "message: ", message);

                  if( auto found = algorithm::find( destination, message.process.pid))
                  {
                     found->state = decltype( found->state())::running;
                  }
                  else
                     log::error( code::casual::invalid_semantics, "failed to correlate entity - ", message.process.pid);


                  algorithm::container::erase( shared->pids, message.process.pid);

                  if( shared->pids.empty())
                     return task::unit::Dispatch::done;
                  else 
                     return task::unit::Dispatch::pending;

               };

               state.task.coordinator.then( task::create::unit( 
                  task::create::action( std::move( task_name), std::move( spawn_action)),
                  std::move( connect_handle),
                  std::move( configuration_reply),
                  exit_handle( shared)
               ));
            }

            void added_groups( State& state, std::vector< casual::configuration::model::queue::Group> groups)
            {
               Trace trace{ "queue::manager::configuration::conform::local::added_groups"};

               added_entities< queue::ipc::message::group::Connect, queue::ipc::message::group::configuration::update::Request>( 
                  state, std::move( groups), state.groups, "added_groups");
            }

            void added_forward_groups( State& state, std::vector< casual::configuration::model::queue::forward::Group> groups)
            {
               Trace trace{ "queue::manager::configuration::conform::local::added_forward_groups"};

               added_entities< queue::ipc::message::forward::group::Connect, queue::ipc::message::forward::group::configuration::update::Request>( 
                  state, std::move( groups), state.forward.groups, "added_forward_groups");
            }

            void added_fanout_groups( State& state, std::vector< casual::configuration::model::queue::fanout::Group> groups)
            {
               Trace trace{ "queue::manager::configuration::conform::local::added_fanout_groups"};

               added_entities< queue::ipc::message::fanout::group::Connect, queue::ipc::message::fanout::group::configuration::update::Request>( 
                  state, std::move( groups), state.fanout.groups, "added_fanout_groups");
            }

            void removed_entities( State& state, auto configuration, auto& entities, std::string task_name)
            {
               Trace trace{ "queue::manager::configuration::conform::local::removed_entities"};

               log::debug( "state.task.coordinator: ", state.task.coordinator);
               log::debug( "configuration: ", configuration);
               log::debug( "entities: ", entities);

               struct Shared
               {
                  std::vector< strong::process::id> pids;
               };

               auto shared = std::make_shared< Shared>();

               auto shutdown_action = [ &state, &entities, shared, configuration = std::move( configuration)]( task::unit::id)
               {
                  Trace trace{ "queue::manager::configuration::conform::local::removed_entities shutdown_action"};

                  auto send_shutdown = [ &state]( auto& entity)
                  { 
                     entity.state = state::entity::Lifetime::shutdown;
                     
                     if( entity.process.ipc)
                        state.multiplex.send( entity.process.ipc, common::message::shutdown::Request{ common::process::handle()});
                     else
                        signal::send( entity.process.pid, common::code::signal::terminate);

                     return entity.process.pid;
                  };

                  shared->pids = algorithm::transform_if( entities, send_shutdown, contains_alias( configuration));

                  if( shared->pids.empty())
                     return task::unit::action::Outcome::abort;
                  return task::unit::action::Outcome::success;
               };

               state.task.coordinator.then( task::create::unit( 
                  task::create::action( task_name, std::move( shutdown_action)),
                  exit_handle( shared)
               ));

            }

            void removed_groups( State& state, std::vector< casual::configuration::model::queue::Group> groups)
            {
               Trace trace{ "queue::manager::configuration::conform::local::removed_groups"};

               removed_entities( state, std::move( groups), state.groups, "removed_groups");
            }

            void removed_forward_groups( State& state, std::vector< casual::configuration::model::queue::forward::Group> groups)
            {
               Trace trace{ "queue::manager::configuration::conform::local::removed_forward_groups"};

               removed_entities( state, std::move( groups), state.forward.groups, "removed_forward_groups");
            }

            void removed_fanout_groups( State& state, std::vector< casual::configuration::model::queue::fanout::Group> groups)
            {
               Trace trace{ "queue::manager::configuration::conform::local::removed_fanout_groups"};

               removed_entities( state, std::move( groups), state.fanout.groups, "removed_fanout_groups");
            }

            template< typename ConfigurationRequest>
            void modified_entities( State& state, auto configuration, auto& entities, std::string task_name)
            {
               Trace trace{ "queue::manager::configuration::conform::local::modified_entities"};

               using ConfigurationReply = common::message::reverse::type_t< ConfigurationRequest>;

               struct Shared
               {
                  std::vector< strong::process::id> pids;
               };

               auto shared = std::make_shared< Shared>();

               auto update_action = [ &state, &entities, shared, configuration = std::move( configuration)]( task::unit::id)
               {
                  Trace trace{ "queue::manager::configuration::conform::local::modified_entities update_action"};

                  for( auto& group : configuration)
                  {
                     if( auto found = algorithm::find( entities, group.alias))
                     {
                        // TODO - the groups should have the responsibility for it's total configuration.
                        //   I think we should move the configuration to the group, and let the group tell 
                        //   us (the manager) what to do. This way we can have a more clear separation of
                        //   concerns. The group should be the one that knows what queues it has and what 
                        //   configuration they have.

                        // set new configuration for the entity
                        found->configuration = group;

                        ConfigurationRequest request{ common::process::handle()};
                        request.model = group;
                        state.multiplex.send( found->process.ipc, request);
                        shared->pids.push_back( found->process.pid);
                     }
                  }
                  if( shared->pids.empty())
                     return task::unit::action::Outcome::abort;
                  return task::unit::action::Outcome::success;
               };

               auto update_reply_handle = [ shared]( task::unit::id, const ConfigurationReply& message)
               {
                  algorithm::container::erase( shared->pids, message.process.pid);

                  if( shared->pids.empty())
                     return task::unit::Dispatch::done;
                  else 
                     return task::unit::Dispatch::pending;
               };

               state.task.coordinator.then( task::create::unit( 
                  task::create::action( std::move( task_name), std::move( update_action)),
                  std::move( update_reply_handle),
                  exit_handle( shared)
               ));
            }


            void modified_groups( State& state, std::vector< casual::configuration::model::queue::Group> groups)
            {
               Trace trace{ "queue::manager::configuration::conform::local::modified_groups"};

               modified_entities< queue::ipc::message::group::configuration::update::Request>( state, std::move( groups), state.groups, "modified_groups");
            }

            void modified_forward_groups( State& state, std::vector< casual::configuration::model::queue::forward::Group> groups)
            {
               Trace trace{ "queue::manager::configuration::conform::local::modified_forward_groups"};

               modified_entities< queue::ipc::message::forward::group::configuration::update::Request>( state, std::move( groups), state.forward.groups, "modified_forward_groups");
            }

            void modified_fanout_groups( State& state, std::vector< casual::configuration::model::queue::fanout::Group> groups)
            {
               Trace trace{ "queue::manager::configuration::conform::local::modified_fanout_groups"};

               modified_entities< queue::ipc::message::fanout::group::configuration::update::Request>( state, std::move( groups), state.fanout.groups, "modified_fanout_groups");

            }
            
         } // <unnamed>
      } // local

      void conform( State& state, casual::configuration::model::queue::Model current, casual::configuration::model::queue::Model wanted)
      {
          Trace trace{ "queue::manager::configuration::conform"};

          state.note = wanted.note;

          log::debug( "state.task.coordinator: ", state.task.coordinator);

          auto group_change = casual::configuration::model::change::concrete::calculate( current.groups, wanted.groups);
          auto forward_change = casual::configuration::model::change::concrete::calculate( current.forward.groups, wanted.forward.groups);
          auto fanout_change = casual::configuration::model::change::concrete::calculate( current.fanout.groups, wanted.fanout.groups);

          log::debug( "group_change: ", group_change);
          log::debug( "forward_change: ", forward_change);
          log::debug( "fanout_change: ", fanout_change);

         // remove/shutdown first fanouts, then forwards, then groups
         {
            if( ! std::empty( fanout_change.removed))
               local::removed_fanout_groups( state, std::move( fanout_change.removed));
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
            if( ! std::empty( fanout_change.added))
               local::added_fanout_groups( state, std::move( fanout_change.added));
         }

         // modified 
         {
            if( ! std::empty( fanout_change.modified))
               local::modified_fanout_groups( state, std::move( fanout_change.modified));
            if( ! std::empty( forward_change.modified))
               local::modified_forward_groups( state, std::move( forward_change.modified));
            if( ! std::empty( group_change.modified))
               local::modified_groups( state, std::move( group_change.modified));
         }


         log::debug( "state.task.coordinator: ", state.task.coordinator);

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
