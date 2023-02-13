//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/manager/configuration.h"
#include "domain/manager/state.h"
#include "domain/manager/state/order.h"
#include "domain/manager/transform.h"
#include "domain/manager/task/create.h"
#include "domain/manager/task/event.h"

#include "configuration/message.h"
#include "configuration/model/change.h"

#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/algorithm/container.h"
#include "common/string/compose.h"



namespace casual
{
   using namespace common;
   namespace domain::manager::configuration
   {
      namespace local
      {
         namespace
         {
            namespace detail
            {
               struct Change
               {

                  template< typename T>
                  using Entity = casual::configuration::model::change::Result< range::type_t< std::vector< T>>>;
                  
                  Entity< casual::configuration::model::domain::Group> groups;
                  Entity< casual::configuration::model::domain::Server> servers;
                  Entity< casual::configuration::model::domain::Executable> executables;

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( groups);
                     CASUAL_SERIALIZE( servers);
                     CASUAL_SERIALIZE( executables);
                  )
               };

               auto change( casual::configuration::model::domain::Model& current, casual::configuration::model::domain::Model& wanted)
               {
                  Change result;

                  result.groups = casual::configuration::model::change::calculate( current.groups, wanted.groups, []( auto& l, auto& r){ return l.name == r.name;});
                  result.servers = casual::configuration::model::change::calculate( current.servers, wanted.servers, []( auto& l, auto& r){ return l.alias == r.alias;});
                  result.executables = casual::configuration::model::change::calculate( current.executables, wanted.executables, []( auto& l, auto& r){ return l.alias == r.alias;});

                  return result;
               }

               namespace group
               {
                  auto remove( State& state)
                  {
                     return [&state]( auto& group)
                     {
                        if( auto found = algorithm::find( state.groups, group.name))
                        {
                           auto id = algorithm::container::extract( state.groups, std::begin( found)).id;

                           auto remove_membership = [id]( auto& process)
                           {
                              algorithm::container::trim( process.memberships, algorithm::remove( process.memberships, id));
                           };

                           algorithm::for_each( state.servers, remove_membership);
                           algorithm::for_each( state.executables, remove_membership);
                           algorithm::for_each( state.groups, [id]( auto& group)
                           {
                              algorithm::container::trim( group.dependencies, algorithm::remove( group.dependencies, id));
                           });
                        }
                     };
                  }



                  auto id( State& state)
                  {
                     return [&state]( auto& name) 
                     {
                        if( auto found = algorithm::find( state.groups, name))
                           return found->id;

                        code::raise::error( code::casual::invalid_configuration, "unresolved dependency to group '", name, "'" );
                     };
                  }

                  auto transform( State& state)
                  {
                     return [&state]( auto& group)
                     {
                        manager::state::Group result{ group.name, { state.group_id.master}, group.note};
                        result.dependencies = algorithm::transform( group.dependencies, group::id( state));
                        return result;
                     };
                  }

                  auto modify( State& state)
                  {
                     return [&state]( auto& group)
                     {
                        auto found = algorithm::find( state.groups, group.name);
                        assert( found);

                        found->dependencies = algorithm::transform( group.dependencies, group::id( state));
                        found->note = group.note;
                     };
                  }

               } // group

               namespace entity
               {
                  manager::Task add( State& state, const Change& change)
                  {
                     auto servers = transform::alias( change.servers.added, state.groups);
                     auto executables = transform::alias( change.executables.added, state.groups);

                     auto get_id = []( auto& entity){ return entity.id;};
                     state::dependency::Group group;
                     group.servers = algorithm::transform( servers, get_id);
                     group.executables = algorithm::transform( executables, get_id);

                     algorithm::move( servers, std::back_inserter( state.servers));
                     algorithm::move( executables, std::back_inserter( state.executables));

                     return manager::task::create::scale::aliases( { std::move( group)});
                  }

                  manager::Task remove( State& state, const Change& change)
                  {
                     auto scale_and_get_id = []( auto& entities, auto& removed)
                     { 
                        return algorithm::transform( removed, [&]( auto& removed)
                        {
                           auto found = algorithm::find( entities, removed.alias);
                           assert( found);

                           found->scale( 0);
                           return found->id;
                        });
                     };
                     
                     state::dependency::Group group;
                     group.servers = scale_and_get_id( state.servers, change.servers.removed);
                     group.executables = scale_and_get_id( state.executables, change.executables.removed);

                     return manager::task::create::remove::aliases( { std::move( group)});
                  }

                  manager::Task modify( State& state, const Change& change)
                  {
                     Trace trace{ "domain::manager::configuration::local::detail::entity::modify"};

                     auto modify_and_get_id = [&state]( auto& entities, auto& modified)
                     { 
                        using id_type = decltype( entities.front().id);

                        return algorithm::accumulate( modified, std::vector< id_type>{}, [&]( auto result, auto& configuration)
                        {
                           if( auto found = algorithm::find( entities, configuration.alias))
                           {
                              transform::modify( configuration, *found, state.groups);
                              result.push_back( found->id);
                           }
                           return result;
                        });
                     };
                     
                     state::dependency::Group group;
                     group.servers = modify_and_get_id( state.servers, change.servers.modified);
                     group.executables = modify_and_get_id( state.executables, change.executables.modified);

                     return manager::task::create::scale::aliases( { std::move( group)});
                  }

                  
               } // entity

            } // detail

            std::vector< manager::Task> domain( State& state, casual::configuration::model::domain::Model& wanted)
            {
               Trace trace{ "domain::manager::configuration::local::domain"};

               auto change = detail::change( state.configuration.model.domain, wanted);
               log::line( verbose::log, "change: ", change);
  
               // groups
               {
                  algorithm::for_each( change.groups.removed, detail::group::remove( state));
                  algorithm::transform( change.groups.added, std::back_inserter( state.groups), detail::group::transform( state));
                  algorithm::for_each( change.groups.modified, detail::group::modify( state));
               }

               return algorithm::container::emplace::initialize< std::vector< manager::Task>>( 
                  detail::entity::add( state, change),
                  detail::entity::remove( state, change),
                  detail::entity::modify( state, change));
            }

            auto managers( State& state, const casual::configuration::Model& wanted)
            {
               Trace trace{ "domain::manager::configuration::local::managers"};

               std::vector< manager::Task> result;

               // handle the runtime configuration updates
               { 
                  auto stakeholders = algorithm::filter( state.configuration.stakeholders, state::configuration::stakeholder::runtime());
                  log::line( verbose::log, "stakeholders: ", stakeholders);
                     
                  auto handles = algorithm::transform( stakeholders, []( auto& value){ return value.process;});

                  if( ! handles.empty())
                     result.push_back( manager::task::create::configuration::managers::update( state, wanted, std::move( handles)));
               }

               // restarts for "the rest"
               {
                  std::vector< state::dependency::Group> groups;

                  auto add_singleton = [&]( auto& id, std::string description)
                  {
                     if( auto handle = state.singleton( id))
                        if( auto server = state.server( handle.pid))
                        {
                           auto& group = groups.emplace_back();
                           group.description = std::move( description);
                           group.servers.push_back( server->id);
                        }
                  };

                  // TODO the order?

                  if( state.configuration.model.queue != wanted.queue)
                     add_singleton( communication::instance::identity::queue::manager.id, "casual-queue-manager");
                  if( state.configuration.model.transaction != wanted.transaction)
                     add_singleton( communication::instance::identity::transaction::manager.id, "casual-transaction-manager");
                  if( state.configuration.model.gateway != wanted.gateway)
                     add_singleton( communication::instance::identity::gateway::manager.id, "casual-gateway-manager");

                  result.push_back( manager::task::create::restart::aliases( std::move( groups)));
               }

               return result;
            }

         } // <unnamed>
      } // local

      casual::configuration::Model get( State& state)
      {
         Trace trace{ "domain::manager::configuration::get"};

         auto futures = algorithm::transform( algorithm::filter( state.configuration.stakeholders, state::configuration::stakeholder::supplier()), []( auto& value)
         {
            return communication::device::async::call( value.process.ipc, casual::configuration::message::Request{ common::process::handle()});
         });

         return algorithm::accumulate( futures, casual::domain::manager::transform::model( state), []( auto model, auto& future)
         {
            return set_union( model, std::move( future.get( communication::ipc::inbound::device()).model));
         });
      }

      std::vector< common::strong::correlation::id> post( State& state, casual::configuration::Model wanted)
      {
         Trace trace{ "domain::manager::configuration::post"};

         if( state.runlevel() != decltype( state.runlevel())::running)
         {
            manager::task::event::dispatch( state, [&state]()
            {
               common::message::event::Error event{ process::handle()};
               event.code = code::casual::invalid_semantics;
               event.message = string::compose( "domain-manager is not in a running runlevel ", state.runlevel());
               return event;
            });

            return {};
         }

         auto tasks = local::managers( state, wanted);
         algorithm::move( local::domain( state, wanted.domain), std::back_inserter( tasks));

         log::line( verbose::log, "tasks: ", tasks);
         
         // we use the wanted as our new configuration when 'managers' ask for it.
         state.configuration.model = std::move( wanted);
         
         return state.tasks.add( std::move( tasks));
      }

   } // domain::manager::configuration

} // casual
