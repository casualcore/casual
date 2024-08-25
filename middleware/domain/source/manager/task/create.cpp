//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/manager/task/create.h"
#include "domain/manager/task/event.h"
#include "domain/manager/handle.h"
#include "domain/common.h"

#include "configuration/message.h"

#include "common/algorithm/compare.h"
#include "common/algorithm/container.h"
#include "common/log/stream.h"
#include "common/string/compose.h"


namespace casual
{
   using namespace common;

   namespace domain::manager::task::create
   {
      namespace event
      {
         namespace local
         {
            namespace
            {
               template< typename Event>
               casual::task::Group create( State& state, std::string description, common::unique_function< void( State&)> done)
               {
                  log::line( verbose::log, "description: ", description);

                  // create the task unit that will be returned
                  auto task = casual::task::create::unit( 
                     [ &state, description, done = std::move( done)]( casual::task::unit::id id) mutable
                     {
                        if( done)
                           exception::guard( [ &state, &done]()
                           {
                              done( state);
                           });

                        manager::task::event::dispatch( state, [ &id, &description]()
                        {
                           Event event{ common::process::handle()};
                           event.correlation = id;
                           event.description = std::move( description);
                           event.state = decltype( event.state)::done;
                           return event;
                        });

                        return casual::task::unit::action::Outcome::abort;
                     }
                  );

                  return casual::task::Group{ std::move( task)};
               }
               
            } // <unnamed>
         } // local

         casual::task::Group parent( State& state, std::string description, common::unique_function< void( State&)> done)
         {
            Trace trace{ "domain::manager::task::create::event::parent"};

            return local::create< common::message::event::Task>( state, std::move( description), std::move( done));
         }

         casual::task::Group sub( State& state, std::string description, common::unique_function< void( State&)> done)
         {
            Trace trace{ "domain::manager::task::create::event::sub"};

            return local::create< common::message::event::sub::Task>( state, std::move( description), std::move( done));
         }
         
      } // event


      namespace restart
      {
         namespace local
         {
            namespace
            {
               auto task_unit( State& state, strong::server::id id)
               {
                  // we use the _auto restart_ functionality, and set the server restart to true for the 
                  // duration of the task, and then back to the origin (which could be true)
                  struct Shared
                  {
                     Shared( State& state, strong::server::id id) : state{ &state}, id{ id} {}

                     State* state{};
                     strong::server::id id;
                     bool origin_restart{};
                     std::vector< process::Handle> processes;
                     std::vector< strong::process::id> spawned_not_connected;
                  };

                  auto shared = std::make_shared< Shared>( state, id);

                  // helper to send done event.
                  static constexpr auto send_done_event = []( const Shared& shared, casual::task::unit::id id)
                  {
                     auto& server = shared.state->entity( shared.id);
                     server.restart = shared.origin_restart;
                     manager::task::event::dispatch( *shared.state, [ id, &server]()
                     {
                        common::message::event::sub::Task event{ common::process::handle()};
                        event.correlation = id;
                        event.description = server.alias;
                        event.state = decltype( event.state)::done;
                        return event;
                     });
                     return casual::task::unit::Dispatch::done;
                  };

                  // the action that will be invoked when the task starts.
                  auto action = casual::task::create::action( [ shared]( casual::task::unit::id id)
                  {
                     auto& server = shared->state->entity( shared->id);
                     shared->processes = algorithm::transform( server.instances, []( auto& instance){ return instance.handle;});

                     if( shared->processes.empty())
                        return casual::task::unit::action::Outcome::abort;

                     shared->origin_restart = std::exchange( server.restart, true);

                     manager::task::event::dispatch( *shared->state, [ id, &server]()
                     {
                        common::message::event::sub::Task event{ common::process::handle()};
                        event.correlation = id;
                        event.description = server.alias;
                        event.state = decltype( event.state)::started;
                        return event;
                     });

                     // start the restart sequence. We start from the back
                     handle::scale::shutdown( *shared->state, range::back( shared->processes));

                     return casual::task::unit::action::Outcome::success;
                  });

                  auto handle_exit = [ shared]( casual::task::unit::id id, const common::message::event::process::Exit& message)
                  {
                     if( auto found = algorithm::find( shared->processes, message.state.pid))
                        algorithm::container::erase( shared->processes, std::begin( found));

                     // check if we got unexpected exit for pending connection
                     if( auto found = algorithm::find( shared->spawned_not_connected, message.state.pid))
                     {
                        shared->spawned_not_connected.erase( std::begin( found));

                        if( range::empty( shared->spawned_not_connected) && range::empty( shared->processes))
                           return send_done_event( *shared, id);

                        // We still keep going with the next
                        handle::scale::shutdown( *shared->state, range::back( shared->processes));
                     }

                     return casual::task::unit::Dispatch::pending;
                  };

                  auto handle_spawned = [ shared]( casual::task::unit::id id, const common::message::event::process::Spawn& message)
                  {
                     auto& server = shared->state->entity( shared->id);
                     if( server.alias == message.alias)
                        algorithm::container::append( message.pids, shared->spawned_not_connected);

                     return casual::task::unit::Dispatch::pending;
                  };

                  auto handle_connected = [ shared]( casual::task::unit::id id, const common::message::domain::process::connect::Request& message)
                  {
                     if( auto found = algorithm::find( shared->spawned_not_connected, message.information.handle.pid))
                     {
                        algorithm::container::erase( shared->spawned_not_connected, std::begin( found));

                        if( range::empty( shared->spawned_not_connected) && range::empty( shared->processes))
                           return send_done_event( *shared, id);

                        // continue to scale down the next
                        handle::scale::shutdown( *shared->state, range::back( shared->processes));

                     }
                     return casual::task::unit::Dispatch::pending;
                  };

                  return casual::task::create::unit( 
                     std::move( action),
                     std::move( handle_exit),
                     std::move( handle_spawned),
                     std::move( handle_connected)
                  );
               }

               auto task_unit( State& state, strong::executable::id id)
               {
                  // we use the _auto restart_ functionality, and set the server restart to true for the 
                  // duration of the task, and then back to the origin (which could be true)
                  struct Shared
                  {
                     Shared( State& state, strong::executable::id id) : state{ &state}, id{ id} {}

                     State* state{};
                     strong::executable::id id;
                     bool origin_restart{};
                     std::vector< strong::process::id> pids;
                  };

                  auto shared = std::make_shared< Shared>( state, id);

                  // helper to send done event.
                  static constexpr auto send_done_event = []( const Shared& shared, casual::task::unit::id id)
                  {
                     auto& executable = shared.state->entity( shared.id);
                     executable.restart = shared.origin_restart;
                     manager::task::event::dispatch( *shared.state, [ id, &executable]()
                     {
                        common::message::event::sub::Task event{ common::process::handle()};
                        event.correlation = id;
                        event.description = executable.alias;
                        event.state = decltype( event.state)::done;
                        return event;
                     });
                     return casual::task::unit::Dispatch::done;
                  };

                  // the action that will be invoked when the task starts.
                  auto action = casual::task::create::action( [ shared]( casual::task::unit::id id)
                  {
                     auto& executable = shared->state->entity( shared->id);
                     shared->pids = algorithm::transform( executable.instances, []( auto& instance){ return instance.handle;});

                     if( shared->pids.empty())
                        return casual::task::unit::action::Outcome::abort;

                     shared->origin_restart = std::exchange( executable.restart, true);

                     manager::task::event::dispatch( *shared->state, [ id, &executable]()
                     {
                        common::message::event::sub::Task event{ common::process::handle()};
                        event.correlation = id;
                        event.description = executable.alias;
                        event.state = decltype( event.state)::started;
                        return event;
                     });

                     // start the restart sequence. We start from the back
                     common::process::terminate( range::back( shared->pids));

                     return casual::task::unit::action::Outcome::success;
                  });

                  auto handle_exit = [ shared]( casual::task::unit::id id, const common::message::event::process::Exit& message)
                  {
                     if( auto found = algorithm::find( shared->pids, message.state.pid))
                        algorithm::container::erase( shared->pids, std::begin( found));

                     return casual::task::unit::Dispatch::pending;
                  };

                  auto handle_spawned = [ shared]( casual::task::unit::id id, const common::message::event::process::Spawn& message)
                  {
                     auto& executable = shared->state->entity( shared->id);
                     if( executable.alias == message.alias)
                     {
                        if( range::empty( shared->pids))
                           return send_done_event( *shared, id);

                        // continue with the next shutdown
                        common::process::terminate( range::back( shared->pids));
                     }

                     return casual::task::unit::Dispatch::pending;
                  };

                  return casual::task::create::unit( 
                     std::move( action),
                     std::move( handle_exit),
                     std::move( handle_spawned)
                  );
               }
            } // <unnamed>
         } // local


         std::vector< casual::task::Group> groups( State& state, std::vector< state::dependency::Group> groups, common::unique_function< void()> done)
         {
            Trace trace{ "domain::manager::task::create::restart::groups"};
            log::line( verbose::log, "groups: ", groups);

            auto result = algorithm::transform( groups, [ &state]( auto& group)
            {
               auto transform_task_unit = [ &state]( auto& id)
               {
                  return local::task_unit( state, id);
               };

               auto units = algorithm::transform( group.servers, transform_task_unit);
               algorithm::transform( group.executables, std::back_inserter( units), transform_task_unit);

               return casual::task::Group{ std::move( units)};
            });

            if( done)
            {
               // add the "done" task.
               result.emplace_back( casual::task::create::unit( 
                  [ done = std::move( done)]( casual::task::unit::id) mutable
                  {
                     done();
                     return casual::task::unit::action::Outcome::abort;
                  }
               ));
            };

            return result;
         }

         std::vector< casual::task::Group> groups( State& state, std::vector< state::dependency::Group> groups)
         {
            Trace trace{ "domain::manager::task::create::restart::groups"};
            log::line( verbose::log, "groups: ", groups);

            return algorithm::transform( groups, [ &state]( auto& group)
            {
               auto transform_task_unit = [ &state]( auto& id)
               {
                  return local::task_unit( state, id);
               };

               auto units = algorithm::transform( group.servers, transform_task_unit);
               algorithm::transform( group.executables, std::back_inserter( units), transform_task_unit);

               return casual::task::Group{ std::move( units)};
            });
         }


      } // restart

      namespace scale
      {
         namespace local
         {
            namespace 
            {
               namespace group
               {
                  auto scale( State& state, const state::dependency::Group& group)
                  {
                     Trace trace{ "domain::manager::task::create::scale::local::group::scale"};
                     log::line( verbose::log, "group: ", group);

                     auto scale_entity = [&state]( auto id)
                     {
                        Trace trace{ "scale_entity"};

                        auto& entity = state.entity( id);
                        log::line( verbose::log, "entity: ", entity);

                        // scale it
                        handle::scale::instances( state, entity);
                     };

                     algorithm::for_each( group.servers, scale_entity);
                     algorithm::for_each( group.executables, scale_entity);
                  }


                  auto group_done( State& state, casual::task::unit::id id, const state::dependency::Group& group)
                  {
                     Trace trace{ "domain::manager::task::create::scale::local::group::group_done"};

                     auto is_done = [ &state]( auto id)
                     {
                        return algorithm::all_of( state.entity( id).instances, []( auto& instance)
                        {
                           using Enum = decltype( instance.state());
                           return algorithm::compare::any( instance.state(), Enum::running, Enum::exit, Enum::disabled, Enum::error);
                        });
                     };

                     if( algorithm::all_of( group.servers, is_done) && algorithm::all_of( group.executables, is_done))
                     {
                        manager::task::event::dispatch( state, [ &id, &group]()
                        {
                           common::message::event::sub::Task event{ common::process::handle()};
                           event.correlation = id;
                           event.description = group.description;
                           event.state = decltype( event.state)::done;
                           return event;
                        });
                        log::line( verbose::log, "group done: ", group);
                        return casual::task::unit::Dispatch::done;
                     }
                     else
                     {
                        log::line( verbose::log, "group pending: ", group);
                        return casual::task::unit::Dispatch::pending;
                     }
                  };
                  

                  casual::task::Group create_task( State& state, common::unique_function< state::dependency::Group( State&)> action)
                  {
                     Trace trace{ "domain::manager::task::create::scale::local::group::create_task"};

                     auto shared = std::make_shared< state::dependency::Group>();

                     return casual::task::create::unit( 
                        casual::task::create::action([ &state, shared, action = std::move( action)]( casual::task::unit::id id) mutable
                        {
                           manager::task::event::dispatch( state, [&]()
                           {
                              common::message::event::sub::Task event{ common::process::handle()};
                              event.correlation = id;
                              event.description = shared->description;
                              event.state = decltype( event.state)::started;
                              return event;
                           });

                           *shared = action( state);

                           group::scale( state, *shared);

                           if( group::group_done( state, id, *shared) == casual::task::unit::Dispatch::done)
                              return casual::task::unit::action::Outcome::abort;
                           return casual::task::unit::action::Outcome::success;
                        }),
                        [ &state, shared]( casual::task::unit::id id, const common::message::event::process::Exit& message)
                        {
                           return group::group_done( state, id, *shared);
                        },
                        [ &state, shared]( casual::task::unit::id id, const common::message::event::process::Spawn& message)
                        {
                           return group::group_done( state, id, *shared);
                        },
                        [ &state, shared]( casual::task::unit::id id, const common::message::domain::process::connect::Request& message)
                        {
                           return group::group_done( state, id, *shared);
                        }
                     );
                  }
                  
               } // group  
            } // <unnamed>
         } // local


         std::vector< casual::task::Group> groups( State& state, std::vector< state::dependency::Group> groups)
         {
            Trace trace{ "domain::manager::task::create::scale::groups"};
            log::line( verbose::log, "groups: ", groups);

            // just a "wrapper" to hold the group to emulate an action callback
            constexpr static auto create_group_holder = []( auto& group)
            {
               return [ group = std::move( group)]( State&) mutable
               {
                  return std::move( group);
               };
            };

            return algorithm::transform( groups, [ &state]( auto& group)
            {
               return local::group::create_task( state, create_group_holder( group));
            });
         }

         casual::task::Group group( State& state, common::unique_function< state::dependency::Group( State&)> action)
         {
            return local::group::create_task( state, std::move( action));
         }

      } // scale

      namespace configuration::managers
      {
         struct Shared
         {
            Shared( State& state, casual::configuration::Model wanted)
               : state{ &state}, wanted{ std::move( wanted)} {}
            
            struct Correlation
            {
               strong::correlation::id id;
               strong::process::id pid;
            };
            inline friend bool operator == ( const Correlation& lhs, const strong::correlation::id& rhs) { return lhs.id == rhs;}
            inline friend bool operator == ( const Correlation& lhs, strong::process::id rhs) { return lhs.pid == rhs;}

            State* state{};
            casual::configuration::Model wanted;
            std::vector< Correlation> correlations;
         };

         std::vector< casual::task::Group> update( State& state, casual::configuration::Model wanted, const std::vector< process::Handle>& destinations)
         {
            Trace trace{ "domain::manager::task::create::configuration::managers::update"};

            auto shared = std::make_shared< Shared>( state, std::move( wanted));

            auto action = [ shared, destinations]( casual::task::unit::id id)
            {
               casual::configuration::message::update::Request request{ process::handle()};
               request.model = std::move( shared->wanted);

               for( auto& destination : destinations)
                  if( auto correlation = shared->state->multiplex.send( destination.ipc, request))
                     shared->correlations.push_back( Shared::Correlation{ correlation, destination.pid});

               return range::empty( shared->correlations) ? casual::task::unit::action::Outcome::abort : casual::task::unit::action::Outcome::success;
            };

            auto handle_update = [ shared]( casual::task::unit::id id, const casual::configuration::message::update::Reply& message)
            {
               algorithm::container::erase( shared->correlations, message.correlation);
               return range::empty( shared->correlations) ? casual::task::unit::Dispatch::done : casual::task::unit::Dispatch::pending;
            };

            auto handle_exit = [ shared]( casual::task::unit::id id, const common::message::event::process::Exit& message)
            {
               algorithm::container::erase( shared->correlations, message.state.pid);
               return range::empty( shared->correlations) ? casual::task::unit::Dispatch::done : casual::task::unit::Dispatch::pending;
            };

            std::vector< casual::task::Group> result;

            result.emplace_back( casual::task::create::unit( 
               std::move( action),
               std::move( handle_update),
               std::move( handle_exit)
            ));

            return result;
         }
      } // configuration::managers

   } // domain::manager::task::create
} // casual
