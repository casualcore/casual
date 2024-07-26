//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/manager/configuration.h"
#include "gateway/manager/transform.h"

#include "gateway/common.h"

#include "configuration/model/change.h"

#include "common/instance.h"
#include "common/event/send.h"

namespace casual
{
   using namespace common;

   namespace gateway::manager::configuration
   {
      namespace local
      {
         namespace
         {
            namespace spawn
            {
               strong::process::id process( const std::string& alias, const std::filesystem::path& path)
               {
                  try
                  { 
                     auto pid = common::process::spawn(
                        path,
                        {},
                        // To set the alias for the spawned process
                        { instance::variable( instance::Information{ alias})});

                     // Send event
                     {
                        common::message::event::process::Spawn event{ common::process::handle()};
                        event.path = path;
                        event.alias = alias;
                        event.pids.push_back( pid);
                        common::event::send( event);
                     }

                     return pid;
                     
                  }
                  catch( ...)
                  {
                     event::error::send( code::casual::invalid_path, "failed to spawn '", path, "' - ", exception::capture());
                  }
                  return {};
               }

               void group( auto& group)
               {
                  group.process.pid = spawn::process( group.configuration.alias, state::executable::path( group));
               };

               auto group()
               {
                  return []( auto& group)
                  {
                     spawn::group( group);
                  };
               }

            } // spawn

            auto shutdown( State& state, const common::process::Handle& process)
            {
               if( process.ipc)
                  communication::device::blocking::optional::send( process.ipc, common::message::shutdown::Request{ common::process::handle()});
               else 
                  signal::send( process.pid, code::signal::terminate);
            }

            template< typename M>
            auto message_process( auto shared)
            {
               return [ shared]( task::unit::id, const M& message)
               {
                  Trace trace{ "gateway::manager::configuration::conform::local::message_process"};

                  algorithm::container::erase( shared->pids, message.process.pid);
                  log::line( verbose::log, "pids left: ", shared->pids);

                  if( shared->pids.empty())
                     return task::unit::Dispatch::done;
                  else 
                     return task::unit::Dispatch::pending;
               };
            }

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

            task::Unit modified_outbound_groups( State& state, std::vector< casual::configuration::model::gateway::outbound::Group> configuration)
            {
               Trace trace{ "gateway::manager::configuration::conform::local::modified_outbound_groups"};

               struct Shared
               {
                  std::vector< strong::process::id> pids;
               };
        
               auto shared = std::make_shared< Shared>();

               auto action = [ &state, shared, configuration = std::move( configuration)]( task::unit::id)
               {
                  Trace trace{ "gateway::manager::configuration::conform::local::modified_inbound_groups action"};

                  shared->pids = algorithm::accumulate( configuration, std::move( shared->pids), [ &state]( auto result, auto& configuration)
                  {
                     if( auto found = algorithm::find( state.outbound.groups, configuration.alias))
                     {
                        message::outbound::configuration::update::Request request{ common::process::handle()};
                        request.model = configuration;
                        communication::device::blocking::optional::send( found->process.ipc, request);
                        result.push_back( found->process.pid);
                     }
                     return result;
                  });

                  if( std::empty( shared->pids))
                     return task::unit::action::Outcome::abort;

                  return task::unit::action::Outcome::success;
               };

               return task::create::unit(
                  task::create::action( "modified_outbound_groups", std::move( action)),
                  message_process< message::outbound::configuration::update::Reply>( shared),
                  exit_handle( shared)
               );
            }

            task::Unit modified_inbound_groups( State& state, std::vector< casual::configuration::model::gateway::inbound::Group> configuration)
            {
               Trace trace{ "gateway::manager::configuration::conform::local::modified_inbound_groups"};

               struct Shared
               {
                  std::vector< strong::process::id> pids;
               };
        
               auto shared = std::make_shared< Shared>();

               auto action = [ &state, shared, configuration = std::move( configuration)]( task::unit::id)
               {
                  Trace trace{ "gateway::manager::configuration::conform::local::modified_inbound_groups action"};

                  shared->pids = algorithm::accumulate( configuration, std::move( shared->pids), [ &state]( auto result, auto& configuration)
                  {
                     if( auto found = algorithm::find( state.inbound.groups, configuration.alias))
                     {
                        message::inbound::configuration::update::Request request{ common::process::handle()};
                        request.model = configuration;
                        communication::device::blocking::optional::send( found->process.ipc, request);
                        result.push_back( found->process.pid);
                     }

                     return result;
                  });

                  if( std::empty( shared->pids))
                     return task::unit::action::Outcome::abort;

                  return task::unit::action::Outcome::success;
               };

               return task::create::unit(
                  task::create::action( "modified_inbound_groups", std::move( action)),
                  message_process< message::inbound::configuration::update::Reply>( shared),
                  exit_handle( shared)
               );
            }

            task::Unit removed_outbound_groups( State& state, std::vector< casual::configuration::model::gateway::outbound::Group> configuration)
            {
               Trace trace{ "gateway::manager::configuration::conform::local::removed_outbound_groups"};

               struct Shared
               {
                  std::vector< strong::process::id> pids;
               };
        
               auto shared = std::make_shared< Shared>();

               auto action = [ &state, shared, configuration = std::move( configuration)]( task::unit::id)
               {
                  Trace trace{ "gateway::manager::configuration::conform::local::removed_outbound_groups action"};

                  shared->pids = algorithm::accumulate( configuration, std::move( shared->pids), [ &state]( auto result, auto& configuration)
                  {
                     if( auto found = algorithm::find( state.outbound.groups, configuration.alias))
                     {
                        shutdown( state, found->process);
                        result.push_back( found->process.pid);
                     }

                     return result;
                  });

                  if( std::empty( shared->pids))
                     return task::unit::action::Outcome::abort;

                  return task::unit::action::Outcome::success;
               };

               return task::create::unit(
                  task::create::action( "removed_outbound_groups", std::move( action)),
                  exit_handle( shared)
               );
            }

            task::Unit removed_inbound_groups( State& state, std::vector< casual::configuration::model::gateway::inbound::Group> configuration)
            {
               Trace trace{ "gateway::manager::configuration::conform::local::removed_inbound_groups"};

               struct Shared
               {
                  std::vector< strong::process::id> pids;
               };
        
               auto shared = std::make_shared< Shared>();

               auto action = [ &state, shared, configuration = std::move( configuration)]( task::unit::id)
               {
                  Trace trace{ "gateway::manager::configuration::conform::local::removed_inbound_groups action"};

                  shared->pids = algorithm::accumulate( configuration, std::move( shared->pids), [ &state]( auto result, auto& configuration)
                  {
                     if( auto found = algorithm::find( state.inbound.groups, configuration.alias))
                     {
                        shutdown( state, found->process);
                        result.push_back( found->process.pid);
                     }

                     return result;
                  });

                  if( std::empty( shared->pids))
                     return task::unit::action::Outcome::abort;

                  return task::unit::action::Outcome::success;
               };

               return task::create::unit(
                  task::create::action( "removed_inbound_groups", std::move( action)),
                  exit_handle( shared)
               );
            }

            task::Unit added_outbound_groups( State& state, std::vector< casual::configuration::model::gateway::outbound::Group> configuration)
            {
               Trace trace{ "gateway::manager::configuration::conform::local::added_outbound_groups"};

               struct Shared
               {
                  std::vector< strong::process::id> pids;
               };

               auto shared = std::make_shared< Shared>();

               auto action = [ &state, shared, configuration = std::move( configuration)]( task::unit::id)
               {
                  Trace trace{ "gateway::manager::configuration::conform::local::added_outbound_groups action"};

                  auto groups = algorithm::transform( configuration, []( auto& group)
                  {
                     manager::state::outbound::Group result;
                     result.configuration = group;
                     return result;
                  });

                  auto valid_pid = []( auto& group){ return predicate::boolean( group.process.pid);};

                  // spawn the groups
                  auto valid_groups = algorithm::filter( algorithm::for_each( groups, spawn::group()), valid_pid);

                  if( std::empty( valid_groups))
                     return task::unit::action::Outcome::abort;

                  shared->pids = algorithm::transform( valid_groups, []( auto& group){ return group.process.pid;});

                  algorithm::move( valid_groups, std::back_inserter( state.outbound.groups));
                  
                  return task::unit::action::Outcome::success;
               };

               return task::create::unit(
                  task::create::action( "added_outbound_groups", std::move( action)),
                  message_process< message::outbound::configuration::update::Reply>( shared),
                  exit_handle( shared)
               );
            }

            task::Unit added_inbound_groups( State& state, std::vector< casual::configuration::model::gateway::inbound::Group> configuration)
            {
               Trace trace{ "gateway::manager::configuration::conform::local::added_inbound_groups"};

               struct Shared
               {
                  std::vector< strong::process::id> pids;
               };

               auto shared = std::make_shared< Shared>();

               auto action = [ &state, shared, configuration = std::move( configuration)]( task::unit::id)
               {
                  Trace trace{ "gateway::manager::configuration::conform::local::added_inbound_groups action"};

                  auto groups = algorithm::transform( configuration, []( auto& group)
                  {
                     manager::state::inbound::Group result;
                     result.configuration = group;
                     return result;
                  });

                  auto valid_pid = []( auto& group){ return predicate::boolean( group.process.pid);};

                  auto valid_groups = algorithm::filter( algorithm::for_each( groups, spawn::group()), valid_pid);

                  if( std::empty( valid_groups))
                     return task::unit::action::Outcome::abort;

                  shared->pids = algorithm::transform( valid_groups, []( auto& group){ return group.process.pid;});

                  algorithm::move( valid_groups, std::back_inserter( state.inbound.groups));
                  
                  return task::unit::action::Outcome::success;
               };

               return task::create::unit(
                  task::create::action( "added_inbound_groups", std::move( action)),
                  message_process< message::inbound::configuration::update::Reply>( shared),
                  exit_handle( shared)
               );
            }
            
         } // <unnamed>
      } // local

      void conform( State& state, casual::configuration::Model wanted)
      {
         Trace trace{ "gateway::manager::configuration::conform"};

         auto current = transform::configuration( state);

         if( current == wanted.gateway)
         {
            log::line( verbose::log, "nothing to update");
            return;   
         }
         
         auto outbound_change = casual::configuration::model::change::concrete::calculate( std::move( current.outbound.groups), std::move( wanted.gateway.outbound.groups));
         auto inbound_change = casual::configuration::model::change::concrete::calculate( std::move( current.inbound.groups), std::move( wanted.gateway.inbound.groups));

         log::line( verbose::log, "outbound_change: ", outbound_change);
         log::line( verbose::log, "inbound_change: ", inbound_change);

         // add - outbound, inbound
         {
            if( ! std::empty( outbound_change.added))
               state.tasks.then( local::added_outbound_groups( state, std::move( outbound_change.added)));
            if( ! std::empty( inbound_change.added))
               state.tasks.then( local::added_inbound_groups( state, std::move( inbound_change.added)));
         }

         // modified
         {
            if( ! std::empty( outbound_change.modified))
               state.tasks.then( local::modified_outbound_groups( state, std::move( outbound_change.modified)));
            if( ! std::empty( inbound_change.modified))
               state.tasks.then( local::modified_inbound_groups( state, std::move( inbound_change.modified)));

         }

         // removed - inbound, outbound
         {
            if( ! std::empty( inbound_change.removed))
               state.tasks.then( local::removed_inbound_groups( state, std::move( inbound_change.removed)));
            if( ! std::empty( outbound_change.removed))
               state.tasks.then( local::removed_outbound_groups( state, std::move( outbound_change.removed)));
         }

      }

      namespace add
      {
         void group( State& state, casual::configuration::model::gateway::outbound::Group configuration)
         {
            state.tasks.then( local::added_outbound_groups( state, { std::move( configuration)}));
         }

         void group( State& state, casual::configuration::model::gateway::inbound::Group configuration)
         {
            state.tasks.then( local::added_inbound_groups( state, { std::move( configuration)}));
         }

      } // add

      
   } // gateway::manager::configuration
} // casual
