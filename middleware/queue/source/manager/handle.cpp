//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/manager/handle.h"
#include "queue/common/log.h"
#include "queue/manager/admin/server.h"

#include "common/exception/system.h"
#include "common/exception/casual.h"
#include "common/process.h"
#include "common/server/lifetime.h"
#include "common/server/handle/call.h"
#include "common/message/handle.h"
#include "common/event/listen.h"
#include "common/algorithm/compare.h"

#include "common/communication/instance.h"


namespace casual
{
   using namespace common;

   namespace queue
   {
      namespace manager
      {

         namespace ipc
         {
            communication::ipc::inbound::Device& device()
            {
               return communication::ipc::inbound::device();
            }

         } // ipc

         namespace handle
         {

            namespace local
            {
               namespace
               {
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
               } // <unnamed>
            } // local

            namespace process
            {
               void exit( const common::process::lifetime::Exit& exit)
               {
                  Trace trace{ "handle::process::exit"};
                  common::log::line( verbose::log, "exit: ", exit);

                  // we handle it later
                  ipc::device().push( common::message::event::process::Exit{ exit});

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

                           auto groups = common::algorithm::transform( state.groups, []( auto& group){ return group.process;});

                           common::algorithm::for_each(
                              common::server::lifetime::soft::shutdown( groups, std::chrono::seconds{ 1}),
                              []( auto& group)
                              {
                                 handle::process::exit( group);
                              });

                           throw common::exception::casual::Shutdown{};
                        };
                     }

                  } // shutdown

                  namespace lookup
                  {
                     void request( State& state, common::message::queue::lookup::Request& message)
                     {
                        Trace trace{ "handle::lookup::Request"};
                        common::log::line( verbose::log, "message: ", message);

                        auto reply = common::message::reverse::type( message);
                        reply.name = message.name;

                        auto send_reply = common::execute::scope( [&]()
                        {
                           common::log::line( verbose::log, "reply.execution: ", reply.execution);
                           communication::device::blocking::optional::send( message.process.ipc, reply);
                        });

                        auto found = common::algorithm::find( state.queues, message.name);

                        if( found && ! found->second.empty())
                        {
                           log::line( log, "queue found: ", found->second);

                           auto& queue = found->second.front();
                           reply.queue = queue.queue;
                           reply.process = queue.process;
                           reply.order = queue.order;
                        }
                        else
                        {
                           common::log::line( log, "queue not found - ", message.name);
                           // TODO: Check if we have already have pending request for this queue.
                           // If so, we don't need to ask again.
                           // not sure if the semantics holds, so I don't implement it until we know.

                           // We didn't find the queue, let's ask our neighbors.

                           common::message::gateway::domain::discover::Request request;
                           request.correlation = message.correlation;
                           request.domain = common::domain::identity();
                           request.process = common::process::handle();
                           request.queues.push_back(  message.name);

                           if( communication::device::blocking::optional::send( 
                              common::communication::instance::outbound::gateway::manager::optional::device(), std::move( request)))
                           {
                              state.pending.push_back( std::move( message));

                              log::line( log, "pending request added to pending: " , state.pending);

                              // We don't send reply, we'll do it when we get the reply from the gateway.
                              send_reply.release();
                           }
                        }
                     }

                     auto request( State& state)
                     {
                        return [&state]( common::message::queue::lookup::Request& message)
                        {
                           request( state, message);
                        };
                     }

                  } // lookup

                  namespace connect
                  {
                     auto request( State& state)
                     {
                        return [&state]( common::message::queue::connect::Request& message)
                        {
                           Trace trace{ "queue::manager::handle::local::connect::request"};
                           log::line( verbose::log, "message: ", message);

                           auto found = common::algorithm::find( state.groups, message.process.pid);

                           if( ! found)
                           {
                              common::log::line( common::log::category::error, "failed to correlate queue group - ", message.process.pid);
                              return;
                           }

                           auto& group = *found;
                           log::line( verbose::log, "group: ", group);

                           auto reply = common::message::reverse::type( message);
                           reply.name = group.name;

                           auto configuration = state.group_configuration( group.name);

                           if( configuration)
                           {
                              common::log::line( verbose::log, "configuration->queues: ", configuration->queues);
                              
                              common::algorithm::transform( configuration->queues, reply.queues, []( auto& value)
                              {
                                 common::message::queue::Queue result;
                                 result.name = value.name;
                                 result.retry.count = value.retry.count;
                                 result.retry.delay = value.retry.delay;
                                 return result;
                              });
                           }
                           else
                              common::log::line( common::log::category::error, "failed to correlate configuration for group - ", group.name);
                           
                           communication::device::blocking::send( message.process.ipc, reply);
                        
                        };
                     }

                     auto information( State& state)
                     {
                        return [&state]( common::message::queue::Information& message)
                        {
                           Trace trace{ "queue::manager::handle::local::connect::information"};
                           log::line( verbose::log, "message: ", message);

                           if( auto found = common::algorithm::find( state.groups, message.process.pid))
                           {
                              found->process = message.process;
                              log::line( verbose::log, "group: ", *found);
                           }
                           else
                           {
                              // We add the group
                              state.groups.emplace_back( message.name, message.process);
                           }

                           auto add_queue = [&state, process = message.process]( auto& queue)
                           {
                              auto& instances = state.queues[ queue.name];

                              instances.emplace_back( process, queue.id);
                              common::algorithm::stable_sort( instances);
                           };

                           algorithm::for_each( message.queues, add_queue);
                        };
                     }
                  } // connect

                  namespace concurrent
                  {
                     auto advertise( State& state)
                     {
                        return [&state]( common::message::queue::concurrent::Advertise& message)
                        {
                           Trace trace{ "queue::manager::handle::local::concurrent::advertise"};
                           log::line( verbose::log, "message: ", message);

                           state.update( message);

                           if( ! message.queues.add.empty())
                           {
                              // Queues has been added, we check if there are any pending

                              auto split = common::algorithm::stable_partition( state.pending,[&]( auto& pending)
                              {
                                 return ! common::algorithm::any_of( message.queues.add, [&]( auto& queue)
                                 {
                                    return queue.name == pending.name;
                                 });
                              });

                              common::traits::remove_cvref_t< decltype( state.pending)> pending;

                              common::algorithm::move( std::get< 1>( split), pending);
                              common::algorithm::trim( state.pending, std::get< 0>( split));

                              log::line( log, "pending to lookup: ", pending);

                              common::algorithm::for_each( pending, [&]( auto& pending){
                                 lookup::request( state, pending);
                              });
                           }
                        }; 
                     }
                  } // concurrent


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

                              // outbound has already advertised the queues (if any), so we have that handled
                              // check if there are any pending lookups for this reply

                              auto is_correlated = [&message]( auto& pending){ return pending.correlation == message.correlation;};

                              if( auto found = common::algorithm::find_if( state.pending, is_correlated))
                              {
                                 auto request = std::move( *found);
                                 state.pending.erase( std::begin( found));

                                 auto reply = common::message::reverse::type( request);
                                 reply.name = request.name;

                                 auto found_queue = common::algorithm::find( state.queues, request.name);

                                 if( found_queue && ! found_queue->second.empty())
                                 {
                                    auto& queue = found_queue->second.front();
                                    reply.process = queue.process;
                                    reply.queue = queue.queue;
                                 }

                                 communication::device::blocking::optional::send( request.process.ipc, reply);
                              }
                              else
                                 log::line( log, "no pending was found for discovery reply");
                           };
                        }
                     } // discover
                  } // domain  
               } // <unnamed>
            } // local

         } // handle

         namespace startup
         {
            handle::dispatch_type handlers( State& state)
            {
               return common::message::dispatch::handler( ipc::device(),
                  common::message::handle::defaults( ipc::device()),
                  handle::local::connect::request( state),
                  handle::local::connect::information( state),
                  handle::local::process::exit( state)
               );
            }
         } // startup


         handle::dispatch_type handlers( State& state)
         {
            return common::message::dispatch::handler( ipc::device(),
               common::message::handle::defaults( ipc::device()),
               common::event::listener( handle::local::process::exit( state)),
               handle::local::connect::information( state),
               handle::local::connect::request( state),
               handle::local::shutdown::request( state),
               handle::local::lookup::request( state),
               handle::local::concurrent::advertise( state),
               handle::local::domain::discover::request( state),
               handle::local::domain::discover::reply( state),

               common::server::handle::admin::Call{
                  manager::admin::services( state)}
            );
         }

      } // manager
   } // queue
} // casual

