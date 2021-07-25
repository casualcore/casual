//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/manager/task/create.h"
#include "domain/manager/task/event.h"
#include "domain/manager/ipc.h"

#include "domain/manager/handle.h"
#include "domain/common.h"

#include "configuration/message.h"

#include "common/algorithm/compare.h"
#include "common/algorithm/container.h"
#include "common/log/stream.h"


namespace casual
{
   using namespace common;

   namespace domain::manager::task::create
   {

      namespace local
      {
         namespace
         {
            template< typename... Ts>
            auto callbacks( Ts&&... ts)
            {
               return algorithm::container::emplace::initialize< std::vector< manager::task::event::Callback>>( std::forward< Ts>( ts)...);
            }
         } // <unnamed>
      } // local


      namespace restart
      {
         namespace local
         {
            namespace
            {
               namespace progress
               {
                  namespace transform
                  {
                     template< typename E>
                     auto entity( State& state)
                     {
                        return [&state]( auto id)
                        {
                           auto& entity = state.entity( id);
                           
                           E result;
                           result.id = id;
                           result.restart = std::exchange( entity.restart, true);

                           log::line( verbose::log, "entity: ", entity);

                           result.handles = algorithm::transform_if( 
                              entity.instances, 
                              []( auto& i){ return i.handle;},
                              []( auto& i){ return i.state == decltype( i.state)::running;});

                           return result;
                        };

                     }
                  } // transform

                  namespace restore
                  {
                     auto restart( State& state)
                     {
                        return [&state]( auto& current)
                        {
                           auto& entity = state.entity( current.id);
                           entity.restart = current.restart;
                        };
                     }
                  } // restore
                  
               } // progress

               struct Progress
               {
                  Progress( std::vector< state::dependency::Group> groups) : groups{ std::move( groups)} 
                  {
                     algorithm::reverse( this->groups);
                  }

                  //! @returns true if we're 'done'
                  bool next( State& state, const manager::task::Context& context)
                  {
                     Trace trace{ "domain::manager::task::create::restart::aliases::local::Progreass::next"};

                     while( ! groups.empty() && current.empty())
                     {
                        auto group = std::move( groups.back());
                        groups.pop_back();
                        current = Current{ state, std::move( group)};
                     }
                        
                     if( done())
                        return true;

                     current.start( state);
                     
                     return false;
                  }

                  bool done() const { return groups.empty() && current.empty();}

                  
                  bool operator() ( State& state, const manager::task::Context& context, const common::message::event::process::Exit& message)
                  {
                     Trace trace{ "domain::manager::task::create::restart::aliases::local::Progreass::operator() process::Exit"};
                     log::line( verbose::log, "message: ", message);

                     current.handle( state, message);
                     log::line( verbose::log, "progress: ", *this);
                     
                     if( current.empty())
                        return next( state, context);
                     
                     return done();
                  }

                  bool operator() ( State& state, const manager::task::Context& context, const common::message::domain::process::connect::Request& message)
                  {
                     Trace trace{ "domain::manager::task::create::restart::aliases::local::Progreass::operator() process::connect::Request"};
                     log::line( verbose::log, "message: ", message);
                     
                     current.handle( state, message);
                     log::line( verbose::log, "progress: ", *this);

                     if( current.empty())
                        return next( state, context);
                     
                     return done();
                  }

                  struct Current
                  {
                     struct Server
                     {
                        state::Server::id_type id;
                        bool restart = false;
                        std::vector< common::process::Handle> handles;

                        void terminate( State& state)
                        {
                           handle::scale::shutdown( state, { handles.back()});
                        }

                        void handle( State& state, const common::message::event::process::Exit& message)
                        {
                           Trace trace{ "domain::manager::task::create::restart::aliases::local::Progreass::Current::Server::handle process::Exit"};

                           // we 'tag' the ipc to _empty_ so we can correlate when we get the server connect.
                           // note: even if this task is not the one who has triggered the shutdown,
                           //  we still handle the events, and it will work.
                           if( auto found = algorithm::find( handles, message.state.pid))
                              found->ipc = strong::ipc::id{};
                        }

                        void handle( State& state, const common::message::domain::process::connect::Request& message)
                        {
                           Trace trace{ "domain::manager::task::create::restart::aliases::local::Progreass::Current::Server::handle process::connect::Request"};

                           auto& server = state.entity( id);

                           // we're only interested in instances of this server
                           if( ! algorithm::find( server.instances, message.process.pid))
                              return;

                           // a server instance that we're interseted in has connected, remove the tagged ones (one)
                           // and scale "down" another one, if any.

                           auto has_tag = []( auto& handle){ return ! handle.ipc.valid();};
                           
                           algorithm::trim( handles, algorithm::remove_if( handles, has_tag));

                           if( ! handles.empty())
                              terminate( state);
                        }

                        CASUAL_LOG_SERIALIZE (
                           CASUAL_SERIALIZE( id);
                           CASUAL_SERIALIZE( restart);
                           CASUAL_SERIALIZE( handles);
                        )
                     };

                     struct Executable
                     {
                        state::Executable::id_type id;
                        bool restart = false;
                        std::vector< strong::process::id> handles;

                        void terminate()
                        {
                           // start the termination (back to front).
                           common::process::terminate( handles.back());
                        }

                        void handle( const common::message::event::process::Exit& message)
                        {
                           Trace trace{ "domain::manager::task::create::restart::aliases::local::Progreass::Current::Executable::handle process::Exit"};

                           if( auto found = algorithm::find( handles, message.state.pid))
                           {
                              log::line( verbose::log, "found: ", *found);
                              handles.erase( std::begin( found));

                              if( ! handles.empty())
                                 terminate();
                           }
                        }

                        CASUAL_LOG_SERIALIZE (
                           CASUAL_SERIALIZE( id);
                           CASUAL_SERIALIZE( restart);
                           CASUAL_SERIALIZE( handles);
                        )
                     };


                     Current( State& state, state::dependency::Group&& group)
                     {
                        algorithm::transform( group.servers, servers, local::progress::transform::entity< Server>( state));
                        algorithm::transform( group.executables, executables, local::progress::transform::entity< Executable>( state));

                        clean( state);
                     }

                     void start( State& state)
                     {
                        algorithm::for_each( servers, [&state]( auto& entity){ entity.terminate( state);});
                        algorithm::for_each( executables, []( auto& entity){ entity.terminate();});
                     }

                     void handle( State& state, const common::message::event::process::Exit& message)
                     {
                        algorithm::for_each( servers, [&state, &message]( auto& entity){ entity.handle( state, message);});
                        algorithm::for_each( executables, [&message]( auto& entity){ entity.handle( message);});

                        clean( state);
                     }
                     
                     void handle( State& state, const common::message::domain::process::connect::Request& message)
                     {
                        algorithm::for_each( servers, [&state, &message]( auto& entity){ entity.handle( state, message);});

                        clean( state);
                     }

                     void clean( State& state)
                     {
                        auto clean = [&state]( auto& entities)
                        {
                           auto is_active = []( auto& entity){ return ! entity.handles.empty();};

                           auto split = algorithm::partition( entities, is_active);
                           algorithm::for_each( std::get< 1>( split), local::progress::restore::restart( state));
                           algorithm::trim( entities, std::get< 0>( split));
                        };

                        clean( servers);
                        clean( executables);
                     }

                     Current() = default;

                     std::vector< Server> servers;
                     std::vector< Executable> executables;

                     inline bool empty() const { return servers.empty() && executables.empty();}

                     CASUAL_LOG_SERIALIZE (
                        CASUAL_SERIALIZE( servers);
                        CASUAL_SERIALIZE( executables);
                     )

                  };

                  std::vector< state::dependency::Group> groups;
                  Current current;

                  CASUAL_LOG_SERIALIZE({
                     CASUAL_SERIALIZE( groups);
                     CASUAL_SERIALIZE( current);
                  })

               };

               auto task( std::vector< state::dependency::Group> groups)
               {
                  Trace trace{ "domain::manager::task::create::restart::aliases::local::task create"};

                  return [ progress = Progress{ std::move( groups)}]( State& state, const manager::task::Context& context) mutable 
                     -> std::vector< manager::task::event::Callback>
                  {
                     Trace trace{ "domain::manager::task::create::restart::aliases::local::task start"};
                     log::line( verbose::log, "progress", progress);

                     // We start the progress, we could be 'done' directly...
                     if( progress.next( state, context))
                        return {};

                     auto wrapper = [&progress, &state, &context]( auto& message)
                     {
                        return progress( state, context, message);
                     };

                     return create::local::callbacks( 
                        [ wrapper]( const common::message::event::process::Exit& message)
                        {
                           return wrapper( message);
                        },
                        [ wrapper]( const common::message::domain::process::connect::Request& message)
                        {
                           return wrapper( message);
                        }
                     );
                  };

               }
            } // <unnamed>
         } // local

         manager::Task aliases( std::vector< state::dependency::Group> groups)
         {
            Trace trace{ "domain::manager::task::create::restart::aliases"};

            return manager::Task{ "restart aliases", local::task( std::move( groups)),
               {
                  Task::Property::Execution::concurrent,
                  Task::Property::Completion::abortable
               }};
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
                  auto done( State& state, const state::dependency::Group& group)
                  {
                     auto is_done = [&state]( auto id)
                     {
                        auto& entity = state.entity( id);

                        return algorithm::none_of( entity.instances, []( auto& i)
                        {
                           using Enum = decltype( i.state);
                           return algorithm::compare::any( i.state, Enum::scale_out, Enum::scale_in, Enum::spawned);
                        });
                     };

                     return algorithm::all_of( group.servers, is_done) && algorithm::all_of( group.executables, is_done);
                  };

                  auto scale( State& state, const state::dependency::Group& group)
                  {
                     auto scale_entity = [&state]( auto id)
                     {
                        Trace trace{ "domain::manager::task::create::local::scale::group::scale"};

                        auto& entity = state.entity( id);
                        log::line( verbose::log, "entity: ", entity);

                        // scale it
                        handle::scale::instances( state, entity);
                     };

                     algorithm::for_each( group.servers, scale_entity);
                     algorithm::for_each( group.executables, scale_entity);
                  }

                  //! `done_callback` will be called when the task is done
                  template< typename Done> 
                  auto task( std::vector< state::dependency::Group> groups, Done done_callback)
                  {
                     Trace trace{ "domain::manager::task::create::local::scale::group::task create"};
                     log::line( verbose::log, "groups", groups);

                     return [done_callback = std::move( done_callback), groups = std::move( groups)]( State& state, const manager::task::Context& context) mutable 
                        -> std::vector< manager::task::event::Callback>
                     {
                        Trace trace{ "domain::manager::task::create::local::scale::group::task start"};

                        // we remove all 'groups' that are done.
                        algorithm::trim( groups, algorithm::remove_if( groups, [&state]( auto& group){ return group::done( state, group);}));

                        // TODO remove!
                        log::line( verbose::log, "groups", groups);

                        // we could be done directly
                        if( groups.empty())
                           return {};

                        // we take state and groups by reference since we know that the task
                        // will outlive the callbacks
                        auto progress = [context, &state, &groups, &done_callback]()
                        {
                           // returns true if groups are done
                           auto consume = [&state, &context]( auto& groups)
                           {
                              auto [ done, remain] = algorithm::divide_if( groups, [&state]( auto& group){ return ! group::done( state, group);});

                              // send sub-events, if any
                              algorithm::for_each( done, [&]( auto& group)
                              {
                                 manager::task::event::dispatch( state, [&]()
                                 {
                                    common::message::event::sub::Task event{ common::process::handle()};
                                    event.correlation = context.id;
                                    event.description = group.description;
                                    event.state = decltype( event.state)::done;
                                    return event;
                                 });
                              });
                              algorithm::trim( groups, remain);
                              return ! done.empty() && ! groups.empty();
                           };

                           while( consume( groups))
                           {
                              // we've made progress, done's are removed, we scale next
                              group::scale( state, groups.front());
                           }
                           
                           // if groups are empty, we're 'done'
                           if( ! groups.empty())
                              return false;
                           
                           done_callback( state);
                           return true;                   
                        };

                        // we scale the first group (the back)
                        group::scale( state, groups.front());

                        // check the progress.
                        if( progress())
                           return {};

                        return create::local::callbacks( 
                           [ progress]( const common::message::event::process::Exit& message)
                           {
                              log::line( verbose::log, "process exit: ", message);
                              return progress();
                           },
                           [ progress]( const common::message::event::process::Spawn& message)
                           {
                              log::line( verbose::log, "process spawn: ", message);
                              return progress();
                           },
                           [ progress]( const common::message::domain::process::connect::Request& message)
                           {
                              log::line( verbose::log, "server connect: ", message);
                              return progress();
                           },
                           [ progress]( const common::message::event::Idle& message)
                           {
                              log::line( verbose::log, "idle: ", message);
                              return progress();
                           }
                        );

                     };
                  }

                  auto task( std::vector< state::dependency::Group> groups)
                  {
                     return task( std::move( groups), []( auto& state){});
                  }
                  
               } // group  
            } // <unnamed>
         } // local

         manager::Task boot( std::vector< state::dependency::Group> groups, common::strong::correlation::id correlation)
         {
            Trace trace{ "domain::manager::task::create::scale::boot"};

            if( ! correlation)
               correlation = decltype( correlation)::emplace( uuid::make());
            
            // make sure we sett runlevel when we're done.
            auto done_callback = []( State& state)
            {
               state.runlevel = decltype( state.runlevel())::running;
            };

            auto description = string::compose( "boot domain ", common::domain::identity().name);

            return manager::Task{ correlation, std::move( description), local::group::task( std::move( groups), std::move( done_callback)),
            {
               Task::Property::Execution::sequential,
               Task::Property::Completion::mandatory
            }};

         }

         manager::Task shutdown( std::vector< state::dependency::Group> groups)
         {
            Trace trace{ "domain::manager::task::create::scale::shutdown"};

            return manager::Task{ "shutdown domain", local::group::task( std::move( groups)),
            {
               Task::Property::Execution::sequential,
               Task::Property::Completion::mandatory
            }};

         }

         manager::Task aliases( std::string description, std::vector< state::dependency::Group> groups)
         {
            Trace trace{ "domain::manager::task::create::scale::aliases"};
            log::line( verbose::log, "description: ", description);

            return manager::Task{ std::move( description), local::group::task( std::move( groups)),
            {
               Task::Property::Execution::concurrent,
               Task::Property::Completion::abortable
            }};
         }

         manager::Task aliases( std::vector< state::dependency::Group> groups)
         {
            return aliases( "scale aliases", std::move( groups));
         }
      } // scale

      namespace remove
      {
         manager::Task aliases( std::vector< state::dependency::Group> groups)
         {
            auto done_callback = [groups]( State& state)
            {
               auto has_id = []( auto& ids)
               {
                  return [&ids]( auto& entity)
                  {
                     return predicate::boolean( algorithm::find( ids, entity.id));
                  };
               };
               
               for( auto& group: groups)
               {
                  algorithm::trim( state.executables, algorithm::remove_if( state.executables, has_id( group.executables)));
                  algorithm::trim( state.servers, algorithm::remove_if( state.servers, has_id( group.servers)));
               }
            };

            return manager::Task{ "remove alieases", scale::local::group::task( std::move( groups), std::move( done_callback)),
            {
               Task::Property::Execution::sequential,
               Task::Property::Completion::mandatory
            }};
         }
      } // remove

      namespace configuration::managers
      {
         struct Correlation
         {
            strong::correlation::id id;
            process::Handle process;

            friend bool operator == ( const Correlation& lhs, const strong::correlation::id& rhs) { return lhs.id == rhs;}
            friend bool operator == ( const Correlation& lhs, strong::process::id rhs) { return lhs.process == rhs;}
         };

         manager::Task update( casual::configuration::Model wanted, const std::vector< process::Handle>& destinations)
         {
            casual::configuration::message::update::Request request{ process::handle()};
            request.model = std::move( wanted);

            auto correlations = algorithm::transform( destinations, []( auto& process)
            {
               Correlation result;
               result.process = process;
               return result;
            });

            auto invoke = [request = std::move( request), correlations = std::move( correlations)]( State& state, const manager::task::Context& context) mutable 
               -> std::vector< manager::task::event::Callback>
            {
               Trace trace{ "domain::manager::task::create::configuration::managers::update task start"};

               algorithm::remove_if( correlations, [&request]( auto& correlation)
               {
                  if( ( correlation.id = communication::device::blocking::send( correlation.process.ipc, request)))
                     return false;
                  return true;
               });

               if( correlations.empty())
                  return {};

               return create::local::callbacks( 
                  [&correlations]( const casual::configuration::message::update::Reply& message)
                  {
                     algorithm::trim( correlations, algorithm::remove( correlations, message.correlation));
                     return correlations.empty();
                  },
                  [&correlations]( const common::message::event::process::Exit& message)
                  {
                     algorithm::trim( correlations, algorithm::remove( correlations, message.state.pid));
                     return correlations.empty();
                  }
               );
            };

            return manager::Task{ "managers configuration update", std::move( invoke),
            {
               Task::Property::Execution::sequential,
               Task::Property::Completion::mandatory
            }};

         }
      } // configuration::managers

   } // domain::manager::task::create
} // casual