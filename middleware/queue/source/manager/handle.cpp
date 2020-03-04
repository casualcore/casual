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
                        return ! communication::ipc::non::blocking::send( group.queue, message);
                     });

                     // Block for the busy ones, if any
                     algorithm::for_each( busy, [&message]( auto& group)
                     {
                        communication::ipc::blocking::send( group.queue, message);
                     });
                  }
               } // <unnamed>
            } // local

            namespace process
            {
               void Exit::operator () ( const common::message::event::process::Exit& message)
               {
                  common::log::line( verbose::log, "message: ", message);
                  m_state.remove( message.state.pid);
               }

               void exit( const common::process::lifetime::Exit& exit)
               {
                  Trace trace{ "handle::process::exit"};
                  common::log::line( verbose::log, "exit: ", exit);

                  // we handle it later
                  ipc::device().push( common::message::event::process::Exit{ exit});

               }
            } // process

            namespace shutdown
            {
               void Request::operator () ( common::message::shutdown::Request& message)
               {
                  auto groups = common::algorithm::transform( m_state.groups, std::mem_fn( &manager::State::Group::process));

                  common::algorithm::for_each(
                     common::server::lifetime::soft::shutdown( groups, std::chrono::seconds{ 1}),
                     []( auto& group)
                     {
                        process::exit( group);
                     });

                  throw common::exception::casual::Shutdown{};
               }

            } // shutdown

            namespace lookup
            {
               void Request::operator () ( common::message::queue::lookup::Request& message)
               {
                  Trace trace{ "handle::lookup::Request"};
                  common::log::line( verbose::log, "message: ", message);

                  auto reply = common::message::reverse::type( message);


                  auto send_reply = common::execute::scope( [&]()
                  {
                     common::log::line( verbose::log, "reply.execution: ", reply.execution);
                     communication::ipc::blocking::optional::send( message.process.ipc, reply);
                  });

                  auto found = common::algorithm::find( m_state.queues, message.name);

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

                     if( communication::ipc::blocking::optional::send( 
                        common::communication::instance::outbound::gateway::manager::optional::device(), std::move( request)))
                     {
                        m_state.pending.push_back( std::move( message));

                        log::line( log, "pending request added to pending: " , m_state.pending);

                        // We don't send reply, we'll do it when we get the reply from the gateway.
                        send_reply.release();
                     }
                  }
               }

            } // lookup

            namespace connect
            {
               void Request::operator() ( common::message::queue::connect::Request& request)
               {
                  auto found = common::algorithm::find( m_state.groups, request.process.pid);

                  if( found)
                  {
                     auto& group = *found;

                     auto reply = common::message::reverse::type( request);
                     reply.name = group.name;

                     auto configuration = m_state.group_configuration( group.name);

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
                     
                     communication::ipc::blocking::send( request.process.ipc, reply);
                  }
                  else
                  {
                     common::log::line( common::log::category::error, "failed to correlate queue group - ", request.process.pid);
                  }
               }


               void Information::operator () ( common::message::queue::Information& message)
               {

                  for( auto&& queue : message.queues)
                  {
                     auto& instances = m_state.queues[ queue.name];

                     instances.emplace_back( message.process, queue.id);

                     common::algorithm::stable_sort( instances);
                  }

                  auto found = common::algorithm::find( m_state.groups, message.process.pid);

                  if( found)
                  {
                     found->process = message.process;
                     found->connected = true;
                  }
                  else
                  {
                     // We add the group
                     m_state.groups.emplace_back( message.name, message.process);
                  }
               }
            } // connect

            namespace concurrent
            {
               void Advertise::operator () ( common::message::queue::concurrent::Advertise& message)
               {
                  Trace trace{ "handle::domain::Advertise"};
                  log::line( verbose::log, "message: ", message);

                  m_state.update( message);

                  using directive_type = decltype( message.directive);

                  if( common::algorithm::compare::any( message.directive, directive_type::add, directive_type::replace))
                  {
                     // Queues has been added, we check if there are any pending

                     auto split = common::algorithm::stable_partition( m_state.pending,[&]( auto& p){

                        return ! common::algorithm::any_of( message.queues, [&]( auto& q){
                           return q.name == p.name;});
                     });

                     common::traits::remove_cvref_t< decltype( m_state.pending)> pending;

                     common::algorithm::move( std::get< 1>( split), pending);
                     common::algorithm::trim( m_state.pending, std::get< 0>( split));

                     log::line( log, "pending to lookup: ", pending);

                     common::algorithm::for_each( pending, [&]( auto& pending){
                        lookup::Request{ m_state}( pending);
                     });
                  }
               } 
            } // concurrent


            namespace domain
            {
               namespace discover
               {
                  void Request::operator () ( common::message::gateway::domain::discover::Request& message)
                  {
                     Trace trace{ "handle::domain::discover::Request"};
                     common::log::line( verbose::log, "message: ", message);

                     auto reply = common::message::reverse::type( message);

                     reply.process = common::process::handle();
                     reply.domain = common::domain::identity();

                     for( auto& queue : message.queues)
                     {
                        if( common::algorithm::find( m_state.queues, queue))
                        {
                           reply.queues.emplace_back( queue);
                        }
                     }

                     common::log::line( verbose::log, "reply: ", reply);

                     communication::ipc::blocking::send( message.process.ipc, reply);
                  }

                  void Reply::operator () ( common::message::gateway::domain::discover::accumulated::Reply& message)
                  {
                     Trace trace{ "handle::domain::discover::Reply"};
                     common::log::line( verbose::log, "message: ", message);

                     // outbound has already advertised the queues (if any), so we have that handled
                     // check if there are any pending lookups for this reply

                     auto is_correlated = [&message]( auto& pending){ return pending.correlation == message.correlation;};

                     if( auto found = common::algorithm::find_if( m_state.pending, is_correlated))
                     {
                        auto request = std::move( *found);
                        m_state.pending.erase( std::begin( found));

                        auto reply = common::message::reverse::type( request);

                        auto found_queue = common::algorithm::find( m_state.queues, request.name);

                        if( found_queue && ! found_queue->second.empty())
                        {
                           auto& queue = found_queue->second.front();
                           reply.process = queue.process;
                           reply.queue = queue.queue;
                        }

                        communication::ipc::blocking::optional::send( request.process.ipc, reply);
                     }
                     else
                        log::line( log, "no pending was found for discovery reply");
                  }
               } // discover
            } // domain
         } // handle

         handle::dispatch_type handlers( State& state)
         {
            return {
               common::message::handle::defaults( ipc::device()),
               common::event::listener( manager::handle::process::Exit{ state}),
               manager::handle::connect::Information{ state},
               manager::handle::connect::Request{ state},
               manager::handle::shutdown::Request{ state},
               manager::handle::lookup::Request{ state},
               //manager::handle::peek::queue::Request{ m_state},
               manager::handle::concurrent::Advertise{ state},
               manager::handle::domain::discover::Request{ state},
               manager::handle::domain::discover::Reply{ state},

               common::server::handle::admin::Call{
                  manager::admin::services( state)},
            };
         }

      } // manager
   } // queue
} // casual

