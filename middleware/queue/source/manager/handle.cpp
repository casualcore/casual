//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/manager/handle.h"
#include "queue/manager/admin/server.h"
#include "queue/manager/transform.h"
#include "queue/common/log.h"
#include "queue/common/ipc/message.h"
#include "queue/common/ipc.h"


#include "domain/discovery/api.h"

#include "configuration/message.h"

#include "common/process.h"
#include "common/server/lifetime.h"
#include "common/server/handle/call.h"
#include "common/event/listen.h"
#include "common/event/send.h"
#include "common/algorithm/compare.h"
#include "common/message/dispatch/handle.h"
#include "common/message/internal.h"
#include "common/instance.h"
#include "common/communication/instance.h"
#include "common/communication/ipc/send.h"
#include "common/algorithm/is.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include "casual/assert.h"


namespace casual
{
   using namespace common;

   namespace queue::manager::handle
   {
      namespace local
      {
         namespace
         {

            namespace pending::lookups
            {
               //! check if there are pending lookups that can be replied 
               void check( State& state)
               {
                  Trace trace{ "queue::manager::handle::local::pending::lookups::check"};
                  log::line( verbose::log, "pending: ", state.pending.lookups);

                  auto lookup_replied = [ &state]( auto& lookup)
                  {
                     if( auto queue = state.queue( lookup.name))
                     {
                        auto reply = common::message::reverse::type( lookup);
                        reply.name = lookup.name;
                        reply.queue = queue->queue;
                        reply.process = queue->process;
                        reply.order = queue->order;

                        return predicate::boolean( state.multiplex.send( lookup.process.ipc, reply));
                     }
                     return false;
                  };

                  algorithm::container::trim( state.pending.lookups, algorithm::remove_if( state.pending.lookups, lookup_replied));
               }

               void discard( State& state)
               {
                  Trace trace{ "queue::manager::handle::local::pending::lookups::discard"};

                  auto pending = std::exchange( state.pending.lookups, {});
                  log::line( verbose::log, "pending: ", pending);

                  auto discard_lookup = [ &state]( auto& lookup)
                  {
                     auto reply = common::message::reverse::type( lookup);
                     reply.name = lookup.name;
                     state.multiplex.send( lookup.process.ipc, reply);
                  };

                  algorithm::for_each( pending, discard_lookup);
               }


            } // pending::lookups

            namespace event
            {
               namespace process
               {
                  auto exit( State& state)
                  {
                     return [ &state]( const common::message::event::process::Exit& event)
                     {
                        Trace trace{ "queue::manager::handle::local::event::process::exit"};
                        common::log::line( verbose::log, "event: ", event);

                        state.task.coordinator( event);
                     };
                  }
               } // process
               
            } // event

            namespace shutdown
            {
               namespace detail
               {
                  template< typename Es>
                  auto action( State& state, Es& entities)
                  {
                     return [ &state, &entities]( task::unit::id id)
                     {
                        Trace trace{ "queue::manager::handle::local::shutdown::action"};
                        common::log::line( verbose::log, "entities: ", entities);

                        state::task::State task_state;
                        task_state.id = id;
                        task_state.pids = algorithm::transform( entities, [ &state]( auto& entity)
                        {
                           if( entity.process.ipc)
                              state.multiplex.send( entity.process.ipc, common::message::shutdown::Request{ common::process::handle()});
                           else
                              signal::send( entity.process.pid, code::signal::terminate);

                           return entity.process.pid;
                        });

                        common::log::line( verbose::log, "task_state: ", task_state);

                        if( task_state.pids.empty())
                           return task::unit::action::Outcome::abort;

                        state.task.states.push_back( std::move( task_state));
                        return task::unit::action::Outcome::success;
                     };
                  };

                  auto handler( State& state)
                  {
                     return [ &state]( task::unit::id id, const common::message::event::process::Exit& event)
                     {
                        Trace trace{ "queue::manager::handle::local::shutdown::request shutdown_handler"};

                        auto task_state = casual::assertion( algorithm::find( state.task.states, id));
                        algorithm::container::erase( task_state->pids, event.state.pid);

                        common::log::line( verbose::log, "task_state: ", task_state);

                        state.remove( event.state.pid);

                        if( ! task_state->pids.empty())
                           return task::unit::Dispatch::pending;
                        else
                           state.task.states.erase( std::begin( task_state));
                        return task::unit::Dispatch::done;
                     };
                  }
                  
               } // detail

               
               auto request( State& state)
               {
                  return [&state]( common::message::shutdown::Request& message)
                  {
                     Trace trace{ "queue::manager::handle::local::shutdown::request"};
                     common::log::line( verbose::log, "message: ", message);

                     state.runlevel = decltype( state.runlevel())::shutdown;

                     local::pending::lookups::discard( state);

                     // create coordinated tasks, first the forwards, then the queue groups
                     state.task.coordinator.then( task::create::unit( 
                        task::create::action( shutdown::detail::action( state, state.forward.groups)),
                        shutdown::detail::handler( state)
                     )).then( task::create::unit( 
                        task::create::action( shutdown::detail::action( state, state.groups)),
                        shutdown::detail::handler( state)
                     ));
                  };
               }

            } // shutdown

            namespace lookup
            {
               namespace detail::dispatch::lookup
               {
                  void reply_absent_queue( State& state, queue::ipc::message::lookup::Request&& message)
                  {
                     Trace trace{ "queue::manager::handle::local::lookup::detail::dispatch::lookup::reply_absent_queue"};

                     auto reply = common::message::reverse::type( message);
                     reply.name = std::move( message.name);
                     state.multiplex.send( message.process.ipc, reply);
                  }

                  void absent_queue( State& state, queue::ipc::message::lookup::Request& message)
                  {
                     using Enum = decltype( message.context.semantic);

                     switch( message.context.semantic)
                     {
                        case Enum::direct:
                        {
                           reply_absent_queue( state, std::move( message));
                           return;
                        }
                        case Enum::wait:
                        {
                           // either we've sent a discovery or the lookup want's to wait (possible for ever).
                           state.pending.lookups.push_back( std::move( message));
                           return;
                        }
                     }
                     casual::terminate( "unknown value for message.context.semantic: ", std::to_underlying( message.context.semantic));
                  }

                  void discovery( State& state, queue::ipc::message::lookup::Request& message)
                  {                     
                     Trace trace{ "queue::manager::handle::local::lookup::detail::dispatch::lookup::discovery"};
                     
                     casual::domain::discovery::request( {}, { message.name}, message.correlation);
                     state.pending.lookups.push_back( std::move( message));
                  }

                  void reply( State& state, const casual::queue::manager::state::Queue& queue, const queue::ipc::message::lookup::Request& message)
                  {
                     auto reply = common::message::reverse::type( message);
                     reply.name = message.name;
                     reply.queue = queue.queue;
                     reply.process = queue.process;
                     reply.order = queue.order;
                     state.multiplex.send( message.process.ipc, reply);
                  }

                  bool internal_only( State& state, queue::ipc::message::lookup::Request& message)
                  {
                     if( auto queue = state.local_queue( message.name))
                     {
                        dispatch::lookup::reply( state, *queue, message);
                        return true;
                     }

                     return false;
                  }

                  bool internal_external( State& state, queue::ipc::message::lookup::Request& message)
                  {
                     if( auto queue = state.queue( message.name))
                     {
                        dispatch::lookup::reply( state, *queue, message);
                        return true;
                     }

                     return false;
                  }

               } // detail::dispatch::lookup

               auto request( State& state)
               {
                  return [ &state]( queue::ipc::message::lookup::Request& message)
                  {
                     Trace trace{ "queue::manager::local::handle::lookup::request"};
                     common::log::line( verbose::log, "message: ", message);

                     if( state.runlevel > decltype( state.runlevel())::running)
                     {
                        common::log::line( log, "runlevel [", state.runlevel, "] - action: reply with absent queue");
                        local::lookup::detail::dispatch::lookup::reply_absent_queue( state, std::move( message));
                        return;
                     }

                     using Enum = decltype( message.context.requester);

                     switch( message.context.requester)
                     {
                        case Enum::internal:
                           if( ! detail::dispatch::lookup::internal_external( state, message))
                              detail::dispatch::lookup::discovery( state, message);
                           return;
                        case Enum::external:
                           if( ! detail::dispatch::lookup::internal_only( state, message))
                              detail::dispatch::lookup::absent_queue( state, message);
                           return;
                        case Enum::external_discovery:
                           if( ! detail::dispatch::lookup::internal_external( state, message))
                              detail::dispatch::lookup::absent_queue( state, message);
                           return;
                     }
                     casual::terminate( "unknown value for message.context.requester: ", std::to_underlying( message.context.requester));
                  };
               }

               namespace discard
               {
                  auto request( State& state)
                  {
                     return [&state]( const queue::ipc::message::lookup::discard::Request& message)
                     {
                        Trace trace{ "handle::lookup::discard::Request"};
                        common::log::line( verbose::log, "message: ", message);

                        auto reply = message::reverse::type( message);

                        if( auto found = algorithm::find( state.pending.lookups, message.correlation))
                        {
                           reply.state = decltype( reply.state)::discarded;
                           state.pending.lookups.erase( std::begin( found));
                        }
                        else
                           reply.state = decltype( reply.state)::replied;

                        state.multiplex.send( message.process.ipc, reply);
                     };
                  }
               } // discard

            } // lookup

            namespace group
            {
               auto connect( State& state)
               {
                  return [&state]( queue::ipc::message::group::Connect& message)
                  {
                     Trace trace{ "queue::manager::handle::local::group::connect"};
                     log::line( verbose::log, "message: ", message);

                     if( auto found = common::algorithm::find( state.groups, message.process.pid))
                     {
                        found->state = decltype( found->state())::connected;
                        found->process = message.process;

                        // send configuration
                        queue::ipc::message::group::configuration::update::Request request{ common::process::handle()};
                        request.model = found->configuration;
                        state.multiplex.send( message.process.ipc, request);
                     }
                     else
                        common::log::line( common::log::category::error, "failed to correlate queue group - ", message.process.pid);
                  };
               }

               namespace configuration::update
               {
                  auto reply( State& state)
                  {
                     return [&state]( queue::ipc::message::group::configuration::update::Reply&& message)
                     {
                        Trace trace{ "queue::manager::handle::local::group::configuration::update::reply"};
                        log::line( verbose::log, "message: ", message);

                        state.update( std::move( message));

                        pending::lookups::check( state);
                     };
                  }
               } // configuration::update

            } // group

            namespace forward
            {
               auto connect( State& state)
               {
                  return [&state]( queue::ipc::message::forward::group::Connect& message)
                  {
                     Trace trace{ "queue::manager::handle::local::forward::connect"};
                     log::line( verbose::log, "message: ", message);

                     if( auto found = common::algorithm::find( state.forward.groups, message.process.pid))
                     {
                        log::line( verbose::log, "found: ", *found);

                        found->state = decltype( found->state())::connected;
                        found->process = message.process;

                        // send configuration
                        queue::ipc::message::forward::group::configuration::update::Request request{ common::process::handle()};
                        request.model = found->configuration;
                        request.groups = state.group_coordinator.config();
                        state.multiplex.send( message.process.ipc, request);
                     }
                     else
                        common::log::line( common::log::category::error, "failed to correlate forward group - ", message.process.pid);
                  };
               }

               namespace configuration::update
               {
                  auto reply( State& state)
                  {
                     return [&state]( queue::ipc::message::forward::group::configuration::update::Reply&& message)
                     {
                        Trace trace{ "queue::manager::handle::local::forward::configuration::update::reply"};
                        log::line( verbose::log, "message: ", message);

                        if( auto found = common::algorithm::find( state.forward.groups, message.process.pid))
                           found->state = decltype( found->state())::running;
                        else
                           common::log::line( common::log::category::error, "failed to correlate forward group - ", message.process.pid);
                     };
                  }
               } // configuration::update
               
            } // forward

            auto advertise( State& state)
            {
               return [&state]( queue::ipc::message::Advertise& message)
               {
                  Trace trace{ "queue::manager::handle::local::concurrent::advertise"};
                  log::line( verbose::log, "message: ", message);

                  state.update( message);

                  pending::lookups::check( state);
               }; 
            }

            namespace domain::discover
            {
               namespace lookup
               {
                  auto request( State& state)
                  {
                     return [ &state]( casual::domain::message::discovery::lookup::Request&& message)
                     {
                        Trace trace{ "queue::manager::handle::local::domain::discovery::internal::request"};
                        common::log::line( verbose::log, "message: ", message);

                        CASUAL_ASSERT( algorithm::is::sorted( message.content.queues) && algorithm::is::unique( message.content.queues));

                        auto get_queue = [ &state, scope = message.scope]( auto& name)
                        {
                           if( scope == decltype( scope)::internal)
                              return state.local_queue( name);
                           else
                              return state.queue( name);
                        };

                        auto reply = common::message::reverse::type( message);

                        for( auto& name : message.content.queues)
                        {
                           if( auto queue = get_queue( name))
                              reply.content.queues.emplace_back( std::move( name), queue->order);
                           else
                              reply.absent.queues.push_back( std::move( name));
                        }
                        
                        common::log::line( verbose::log, "reply: ", reply);

                        state.multiplex.send( message.process.ipc, reply);
                     };
                  }
               } // lookup

               namespace api
               {
                  auto reply( State& state)
                  {
                     return [&state]( casual::domain::message::discovery::api::Reply& message)
                     {
                        Trace trace{ "queue::manager::handle::local::domain::discovery::api::reply"};
                        common::log::line( verbose::log, "message: ", message);

                        pending::lookups::check( state);

                        // we need to reply to the caller that instigated the discovery, if the 
                        // context is direct  (and the lookup did not find what the caller wants, via check::pending::lookups)
                        if( auto found = algorithm::find( state.pending.lookups, message.correlation); found && found->context.semantic == decltype( found->context.semantic)::direct)
                           local::lookup::detail::dispatch::lookup::reply_absent_queue( state, algorithm::container::extract( state.pending.lookups, std::begin( found))); 
                     };
                  }

               } // api

               namespace fetch::known
               {
                  auto request( State& state)
                  {
                     return [&state]( casual::domain::message::discovery::fetch::known::Request& message)
                     {
                        Trace trace{ "queue::manager::handle::local::domain::discover::fetch::known::request"};
                        common::log::line( verbose::log, "message: ", message);

                        auto reply = common::message::reverse::type( message);

                        // all known "remote" queues
                        reply.content.queues = algorithm::accumulate( state.queues, std::move( reply.content.queues), []( auto result, auto& pair)
                        {
                           auto is_remote = []( auto& queue){ return queue.remote();};
                           if( algorithm::find_if( std::get< 1>( pair), is_remote))
                              result.push_back( std::get< 0>( pair));

                           return result;
                        });

                        // all 'wait for ever'
                        for( auto& pending : state.pending.lookups)
                           if( pending.context.semantic == decltype( pending.context.semantic)::wait)
                              reply.content.queues.push_back( pending.name);

                        // make sure we respect invariants
                        algorithm::container::sort::unique( reply.content.queues);

                        common::log::line( verbose::log, "reply: ", reply);

                        state.multiplex.send( message.process.ipc, reply);
                        
                     };
                  }
                  
               } // fetch::known

            } // domain::discover

            namespace configuration
            {
               auto request( State& state)
               {
                  return [&state]( casual::configuration::message::Request& message)
                  {
                     Trace trace{ "handle::domain::handle::local::configuration::request"};
                     common::log::line( verbose::log, "message: ", message);

                     auto reply = common::message::reverse::type( message);

                     reply.model.queue = transform::configuration( state);

                     state.multiplex.send( message.process.ipc, reply);
                  };

               }

            } // configuration
         } // <unnamed>
      } // local

      namespace process
      {
         void exit( State& state, const common::process::lifetime::Exit& exit)
         {
            Trace trace{ "handle::process::exit"};
            common::log::line( verbose::log, "exit: ", exit);

            // one of our own children has died, we send the event.
            // we'll later receive the event from domain-manager, since
            // we listen to process-exit-events
            state.multiplex.send( communication::instance::outbound::domain::manager::device(), common::message::event::process::Exit{ exit}); 

            // we still remove our own as early as possible, to mitigate some noise.
            state.remove( exit.pid);
         }
      } // process

      namespace comply
      {
         void configuration( State& state, casual::configuration::Model model)
         {
            Trace trace{ "queue::manager::handle::comply::configuration"};
            log::line( verbose::log, "model: ", model);

            // TODO maintainence: runtime configuration

            state.note = model.queue.note;
            state.group_coordinator.update( model.domain.groups);

            state.groups = algorithm::transform( model.queue.groups, []( auto& config){ return state::Group{ std::move( config)};});
            state.forward.groups = algorithm::transform( model.queue.forward.groups, []( auto& config){ return state::forward::Group{ std::move( config)};});

            auto spawn = []( auto& entity)
            {
               entity.process.pid = common::process::spawn(
                  state::entity::path( entity),
                  {},
                  { instance::variable( instance::Information{ entity.configuration.alias})});
               entity.state = decltype( entity.state())::spawned;
            };

            algorithm::for_each( state.groups, spawn);
            algorithm::for_each( state.forward.groups, spawn);
            
         }
      } // comply

      void idle( State& state)
      {
         Trace trace{ "queue::manager::handle::idle"};

         if( state.runlevel == state::Runlevel::configuring && state.ready())
         {
            state.runlevel = state::Runlevel::running;
            
            // Connect to domain, and let domain-manager know that we're ready.
            communication::instance::whitelist::connect( communication::instance::identity::queue::manager);
         }
      }

      void abort( State& state)
      {
         Trace trace{ "queue::manager::handle::abort"};
         log::line( verbose::log, "state: ", state);

         state.runlevel = decltype( state.runlevel())::error;

         local::pending::lookups::discard( state);

         auto terminate = [&]( auto& entity)
         {
            signal::send( entity.process.pid, code::signal::terminate);
         };

         algorithm::for_each( state.forward.groups, terminate);
         algorithm::for_each( state.groups, terminate);

      }

      handle::dispatch_type create( State& state)
      {
         return common::message::dispatch::handler( ipc::device(),
            common::message::dispatch::handle::defaults( state),
            
            handle::local::group::connect( state),
            handle::local::group::configuration::update::reply( state),

            handle::local::forward::connect( state),
            handle::local::forward::configuration::update::reply( state),

            handle::local::configuration::request( state),
            
            handle::local::lookup::request( state),
            handle::local::lookup::discard::request( state),
            
            handle::local::advertise( state),
            
            handle::local::domain::discover::lookup::request( state),
            handle::local::domain::discover::api::reply( state),
            handle::local::domain::discover::fetch::known::request( state),

            handle::local::shutdown::request( state),
            common::event::listener( handle::local::event::process::exit( state)),

            common::server::handle::admin::Call{
               manager::admin::services( state)}
         );  
      }

   } // queue::manager::handle
} // casual

