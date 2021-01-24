//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/manager/handle.h"
#include "queue/manager/admin/server.h"
#include "queue/common/log.h"
#include "queue/common/ipc/message.h"

#include "common/process.h"
#include "common/server/lifetime.h"
#include "common/server/handle/call.h"
#include "common/event/listen.h"
#include "common/event/send.h"
#include "common/algorithm/compare.h"
#include "common/message/handle.h"
#include "common/message/gateway.h"


#include "common/code/raise.h"
#include "common/code/casual.h"

#include "common/communication/instance.h"


namespace casual
{
   using namespace common;

   namespace queue::manager
   {

      namespace handle
      {

         namespace local
         {
            namespace
            {
               namespace ipc::manager::optional
               {
                  auto& gateway() { return communication::instance::outbound::gateway::manager::optional::device();}
               } // ipc::manager::optional

               template< typename G, typename M>
               void send( State& state, G&& groups, M&& message)
               {
                  // Try to send it first with no blocking.
                  auto busy = common::algorithm::filter( groups, [&message]( auto& group)
                  {
                     return ! communication::device::non::blocking::send( group.queue, message);
                  });

                  // Block for the busy ones, if any
                  algorithm::for_each( busy, [&message]( auto& group)
                  {
                     communication::device::blocking::send( group.queue, message);
                  });
               }

               namespace check::pending
               {
       
                  //! check if there are pending lookups that can be replied 
                  void lookups( State& state)
                  {
                     Trace trace{ "queue::manager::handle::local::check::pending::lookups"};
                     log::line( verbose::log, "pending: ", state.pending.lookups);

                     auto lookup_replied = [&state]( auto& lookup)
                     {
                        if( auto queue = state.queue( lookup.name))
                        {
                           auto reply = common::message::reverse::type( lookup);
                           reply.name = lookup.name;
                           reply.queue = queue->queue;
                           reply.process = queue->process;
                           reply.order = queue->order;

                           return predicate::boolean( communication::device::blocking::optional::send( lookup.process.ipc, reply));
                        }
                        return false;
                     };

                     algorithm::trim( state.pending.lookups, algorithm::remove_if( state.pending.lookups, lookup_replied));
                  }
       
               } // check::pending
            } // <unnamed>
         } // local

         namespace process
         {
            void exit( const common::process::lifetime::Exit& exit)
            {
               Trace trace{ "handle::process::exit"};
               common::log::line( verbose::log, "exit: ", exit);

               // one of our own children has died, we send the event.
               // we'll later receive the event from domain-manager, since
               // we listen to process-exit-events
               common::event::send( common::message::event::process::Exit{ exit});
            }
         } // process


         namespace local
         {
            namespace
            {
               namespace process
               {
                  auto exit( State& state)
                  {
                     return [&state]( const common::message::event::process::Exit& message)
                     {
                        Trace trace{ "queue::manager::handle::local::process::exit"};
                        common::log::line( verbose::log, "message: ", message);
                        state.remove( message.state.pid);
                     };
                  }
               } // process

               namespace shutdown
               {
                  auto request( State& state)
                  {
                     return [&state]( common::message::shutdown::Request& message)
                     {
                        Trace trace{ "queue::manager::handle::local::shutdown::request"};
                        common::log::line( verbose::log, "message: ", message);

                        state.runlevel = decltype( state.runlevel())::shutdown;

                        auto shutdown = [&]( auto& entity)
                        {
                           if( entity.process.ipc)
                           {
                              communication::device::blocking::optional::send( 
                                 entity.process.ipc,
                                 common::message::shutdown::Request{ common::process::handle()});
                           }
                           else
                              signal::send( entity.process.pid, code::signal::terminate);
                        };

                        algorithm::for_each( state.forward.groups, shutdown);
                        algorithm::for_each( state.groups, shutdown);
                     };
                  }

               } // shutdown

               namespace lookup
               {
                  auto request( State& state)
                  {
                     return [&state]( queue::ipc::message::lookup::Request& message)
                     {
                        Trace trace{ "handle::local::lookup::request"};
                        common::log::line( verbose::log, "message: ", message);

                        auto reply = common::message::reverse::type( message);
                        reply.name = message.name;

                        auto send_reply = common::execute::scope( [&]()
                        {
                           common::log::line( verbose::log, "reply.execution: ", reply.execution);
                           communication::device::blocking::optional::send( message.process.ipc, reply);
                        });

                        if( auto queue = state.queue( message.name))
                        {
                           log::line( log, "queue provider found: ", *queue);
                           reply.queue = queue->queue;
                           reply.process = queue->process;
                           reply.order = queue->order;
                        }
                        else
                        {
                           // We didn't find the queue, let's try to ask our neighbors.

                           common::log::line( log, "queue not found - ", message.name);

                           common::message::gateway::domain::discover::Request request;
                           request.correlation = message.correlation;
                           request.domain = common::domain::identity();
                           request.process = common::process::handle();
                           request.queues.push_back(  message.name);

                           if( communication::device::blocking::optional::send( ipc::manager::optional::gateway(), std::move( request)) 
                              || message.context == decltype( message.context)::wait)
                           {
                              // either we've sent a discovery or the lookup want's to wait (possible for ever).
                              state.pending.lookups.push_back( std::move( message));

                              log::line( log, "pending request added: " , state.pending);

                              // We don't send the 'absent reply'.
                              send_reply.release();
                           }
                        }
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

                           communication::device::blocking::optional::send( message.process.ipc, reply);
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
                           communication::device::blocking::send( message.process.ipc, request);
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

                           check::pending::lookups( state);
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
                           communication::device::blocking::send( message.process.ipc, request);
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

                     check::pending::lookups( state);
                  }; 
               }

               namespace domain
               {
                  namespace discover
                  {
                     auto request( State& state)
                     {
                        return [&state]( common::message::gateway::domain::discover::Request& message)
                        {
                           Trace trace{ "handle::domain::discover::Request"};
                           common::log::line( verbose::log, "message: ", message);

                           auto reply = common::message::reverse::type( message);

                           reply.process = common::process::handle();
                           reply.domain = common::domain::identity();

                           for( auto& queue : message.queues)
                           {
                              if( common::algorithm::find( state.queues, queue))
                                 reply.queues.emplace_back( queue);
                           }

                           common::log::line( verbose::log, "reply: ", reply);

                           communication::device::blocking::send( message.process.ipc, reply);
                        };
                     }

                     auto reply( State& state)
                     {
                        return [&state]( common::message::gateway::domain::discover::accumulated::Reply& message)
                        {
                           Trace trace{ "handle::domain::discover::Reply"};
                           common::log::line( verbose::log, "message: ", message);

                           check::pending::lookups( state);

                           // we need to remove the possible pending, that instigated the request, 
                           // if it's correlated and in _direct-context_

                           algorithm::trim( state.pending.lookups, algorithm::remove_if( state.pending.lookups, [&message]( auto& pending)
                           {
                              return pending.correlation == message.correlation
                                 && pending.context == decltype( pending.context)::direct;
                           }));
                        };
                     }
                  } // discover
               } // domain  
            } // <unnamed>
         } // local

         namespace comply
         {
            void configuration( State& state, casual::configuration::model::queue::Model model)
            {
               Trace trace{ "queue::manager::handle::comply::configuration"};
               log::line( verbose::log, "model: ", model);

               // TODO maintainence: runtime configuration

               state.groups = algorithm::transform( model.groups, []( auto& config){ return state::Group{ std::move( config)};});
               state.forward.groups = algorithm::transform( model.forward.groups, []( auto& config){ return state::forward::Group{ std::move( config)};});

               auto spawn = []( auto& entity)
               {
                  entity.process.pid = common::process::spawn( state::entity::path( entity), {});
                  entity.state = decltype( entity.state())::spawned;
               };

               algorithm::for_each( state.groups, spawn);
               algorithm::for_each( state.forward.groups, spawn);
               
            }
         } // comply
      } // handle


      handle::dispatch_type handlers( State& state)
      {
         return common::message::dispatch::handler( ipc::device(),
            common::message::handle::defaults( ipc::device()),

            common::event::listener( handle::local::process::exit( state)),
            
            handle::local::group::connect( state),
            handle::local::group::configuration::update::reply( state),

            handle::local::forward::connect( state),
            handle::local::forward::configuration::update::reply( state),
            
            handle::local::lookup::request( state),
            handle::local::lookup::discard::request( state),
            
            handle::local::advertise( state),
            
            handle::local::domain::discover::request( state),
            handle::local::domain::discover::reply( state),

            handle::local::shutdown::request( state),
            handle::local::process::exit( state),

            common::server::handle::admin::Call{
               manager::admin::services( state)}
         );
      }

   } // queue::manager
} // casual

