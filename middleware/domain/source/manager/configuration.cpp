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
                  using Entity = casual::configuration::model::change::Result< std::vector< T>>;
                  
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

                  result.groups = casual::configuration::model::change::concrete::calculate( current.groups, wanted.groups, []( auto& l, auto& r){ return l.name == r.name;});
                  result.servers = casual::configuration::model::change::concrete::calculate( current.servers, wanted.servers, []( auto& l, auto& r){ return l.alias == r.alias;});
                  result.executables = casual::configuration::model::change::concrete::calculate( current.executables, wanted.executables, []( auto& l, auto& r){ return l.alias == r.alias;});

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
                        manager::state::Group result{ group.name, { state.group_id.master}, group.note, group.enabled};
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
                        found->enabled = group.enabled;
                     };
                  }

               } // group

               namespace entity
               {
                  casual::task::Group add( State& state, std::shared_ptr< const Change> change)
                  {
                     Trace trace{ "domain::manager::configuration::local::detail::entity::remove"};

                     auto action = [ change]( State& state)
                     {
                        Trace trace{ "domain::manager::configuration::local::detail::entity::add action"};
                        log::line( verbose::log, "change->servers.added: ", change->servers.added);
                        log::line( verbose::log, "change->executables.added: ", change->executables.added);

                        auto servers = transform::alias( change->servers.added, state.groups);
                        auto executables = transform::alias( change->executables.added, state.groups);

                        auto get_id = []( auto& entity){ return entity.id;};
                        state::dependency::Group group;
                        group.description = "add entities";
                        group.servers = algorithm::transform( servers, get_id);
                        group.executables = algorithm::transform( executables, get_id);

                        algorithm::move( servers, std::back_inserter( state.servers));
                        algorithm::move( executables, std::back_inserter( state.executables));

                        return group;
                     };

                     return manager::task::create::scale::group( state, std::move( action));
                  }

                  std::vector< casual::task::Group> remove( State& state, std::shared_ptr< const Change> change)
                  {
                      Trace trace{ "domain::manager::configuration::local::detail::entity::remove"};

                     // The main task that start the scale down
                     auto action = [ change]( State& state)
                     {
                        Trace trace{ "domain::manager::configuration::local::detail::entity::remove action"};

                        log::line( verbose::log, "change->servers.removed: ", change->servers.removed);
                        log::line( verbose::log, "change->executables.removed: ", change->executables.removed);

                        auto scale_in = []( auto& entities, auto& removed)
                        {
                           using id_type = decltype( range::front( entities).id); 
                           return algorithm::accumulate( removed, std::vector< id_type>{}, [&]( auto result, auto& removed)
                           {
                              if( auto found = algorithm::find( entities, removed.alias))
                              {
                                 found->scale( 0);
                                 result.push_back( found->id);
                              }
                              return result;
                           });
                        };
                     
                        state::dependency::Group group;
                        group.description = "scale down and remove";
                        group.servers = scale_in( state.servers, change->servers.removed);
                        group.executables = scale_in( state.executables, change->executables.removed);

                        log::line( verbose::log, "change: ", *change);
                        log::line( verbose::log, "group: ", group);

                        return group;
                     };

                     std::vector< casual::task::Group> result;

                     result.push_back( manager::task::create::scale::group( state, action));

                     // Make sure we actually removes the services/executables. We add another task that will
                     // be executed after the main task above.
                     result.push_back( task::create::event::sub( state, "scale down and remove", [ change]( State& state)
                     {
                        Trace trace{ "domain::manager::configuration::local::detail::entity::remove done-event"};

                        auto has_id = []( auto& aliases)
                        {
                           return [ &aliases]( auto& entity)
                           {
                              return entity.instances.empty() && algorithm::find( aliases, entity.alias);
                           };
                        };

                        algorithm::container::erase_if( state.servers, has_id( change->servers.removed));
                        algorithm::container::erase_if( state.executables, has_id( change->executables.removed));  
                     }));

                     return result;
                  }

                  casual::task::Group modify( State& state, std::shared_ptr< const Change> change)
                  {
                     Trace trace{ "domain::manager::configuration::local::detail::entity::modify"};

                     auto action = [ change]( State& state)
                     {
                        Trace trace{ "domain::manager::configuration::local::detail::entity::modify action"};

                        log::line( verbose::log, "change->servers.modified: ", change->servers.modified);
                        log::line( verbose::log, "change->executables.modified: ", change->executables.modified);

                        auto modify_and_get_id = [ &state]( auto& entities, auto& modified)
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
                        group.description = "modify entities";
                        group.servers = modify_and_get_id( state.servers, change->servers.modified);
                        group.executables = modify_and_get_id( state.executables, change->executables.modified);

                        return group;
                     };

                     return manager::task::create::scale::group( state, std::move( action));
                  }

                  
               } // entity

            } // detail

            std::vector< casual::task::Group> domain( State& state, casual::configuration::model::domain::Model& wanted)
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

               auto shared = std::make_shared< const local::detail::Change>( std::move( change));
               log::line( verbose::log, "shared: ", *shared);

               return algorithm::container::compose( 
                  detail::entity::add( state, shared),
                  detail::entity::remove( state, shared),
                  detail::entity::modify( state, shared));
            }

            std::vector< casual::task::Group> managers( State& state, const casual::configuration::Model& wanted)
            {
               Trace trace{ "domain::manager::configuration::local::managers"};

               std::vector< casual::task::Group> result;

               // handle the runtime configuration updates
               { 
                  auto stakeholders = algorithm::filter( state.configuration.stakeholders, state::configuration::stakeholder::runtime());
                  log::line( verbose::log, "stakeholders: ", stakeholders);
                     
                  auto handles = algorithm::transform( stakeholders, []( auto& value){ return value.process;});

                  if( ! handles.empty())
                     algorithm::container::move::append( manager::task::create::configuration::managers::update( state, wanted, std::move( handles)), result);
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

                  //if( state.configuration.model.transaction != wanted.transaction)
                  //   add_singleton( communication::instance::identity::transaction::manager.id, "casual-transaction-manager");
                  if( state.configuration.model.gateway != wanted.gateway)
                     add_singleton( communication::instance::identity::gateway::manager.id, "casual-gateway-manager");

                  algorithm::container::move::append( manager::task::create::restart::groups( state, std::move( groups)), result);
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

         auto done_event = task::create::event::parent( state, "configuration post");

         auto tasks = local::managers( state, wanted);
         algorithm::move( local::domain( state, wanted.domain), std::back_inserter( tasks));

         // we use the wanted as our new configuration when 'managers' ask for it.
         
         state.configuration.model = std::move( wanted);

         auto result = casual::task::ids( tasks, done_event);
         state.tasks.then( std::move( tasks)).then( std::move( done_event));

         log::line( verbose::log, "state.tasks: ", state.tasks);
         
         return result;
      }

   } // domain::manager::configuration

} // casual
