//!
//! casual
//!

#include "queue/broker/handle.h"
#include "queue/common/log.h"

#include "common/error.h"
#include "common/exception.h"
#include "common/process.h"
#include "common/server/lifetime.h"




namespace casual
{
   namespace queue
   {
      namespace broker
      {

         namespace ipc
         {
            const common::communication::ipc::Helper device()
            {
               static common::communication::ipc::Helper ipc{
                  common::communication::error::handler::callback::on::Terminate
                  {
                     []( const common::process::lifetime::Exit& exit){
                        //
                        // We put a dead process event on our own ipc device, that
                        // will be handled later on.
                        //
                        common::message::domain::process::termination::Event event{ exit};
                        common::communication::ipc::inbound::device().push( std::move( event));
                     }
                  }
               };
               return ipc;
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
                     // TODO: until we get "auto lambdas"
                     using group_type = decltype( *std::begin( groups));

                     //
                     // Try to send it first with no blocking.
                     //

                     auto busy = common::range::partition( groups, [&]( group_type& g)
                           {
                              return ! ipc::device().non_blocking_send( g.queue, message);
                           });

                     //
                     // Block for the busy ones, if any
                     //
                     for( auto&& group : std::get< 0>( busy))
                     {
                        ipc::device().blocking_send( group.queue, message);
                     }

                  }

                  namespace optional
                  {
                     template< typename D, typename M>
                     bool send( D&& device, M&& message)
                     {
                        try
                        {
                           ipc::device().blocking_send( device, std::forward< M>( message));
                           return true;
                        }
                        catch( const common::exception::communication::Unavailable&)
                        {
                           return false;
                        }
                     }
                  } // optional

                  template< typename D, typename M>
                  void reply( D&& device, M&& message)
                  {
                     if( ! optional::send( std::forward< D>( device), std::forward< M>( message)))
                     {
                        common::log::error << "device [" << device << "] unavailable for reply - action: ignore\n";
                     }
                  }



               } // <unnamed>
            } // local

            namespace process
            {
               void Exit::operator () ( message_type& message)
               {
                  apply( message.death);
               }

               void Exit::apply( const common::process::lifetime::Exit& exit)
               {
                  Trace trace{ "handle::process::Exit"};

                  m_state.remove( exit.pid);

               }
            } // process

            namespace shutdown
            {
               void Request::operator () ( message_type& message)
               {
                  std::vector< common::process::Handle> groups;
                  common::range::transform( m_state.groups, groups, std::mem_fn( &broker::State::Group::process));

                  common::range::for_each(
                        common::server::lifetime::soft::shutdown( groups, std::chrono::seconds( 1)),
                        std::bind( &process::Exit::apply, process::Exit{ m_state}, std::placeholders::_1));

                  throw common::exception::Shutdown{ "shutting down", __FILE__, __LINE__};
               }

            } // shutdown

            namespace lookup
            {

               void Request::operator () ( message_type& message)
               {
                  Trace trace{ "handle::lookup::Request"};

                  auto reply = common::message::reverse::type( message);


                  auto send_reply = common::scope::execute( [&](){
                     local::reply( message.process.queue, reply);
                  });

                  auto found =  common::range::find( m_state.queues, message.name);

                  if( found && ! found->second.empty())
                  {
                     auto& queue = found->second.front();
                     reply.queue = queue.queue;
                     reply.process = queue.process;
                  }
                  else
                  {
                     //
                     // TODO: Check if we have already have pending request for this queue.
                     // If so, we don't need to ask again.
                     // not sure if the semantics holds, so I don't implement it until we know.
                     //

                     //
                     // We didn't find the queue, let's ask our neighbors.
                     //

                     common::message::gateway::domain::discover::external::Request request;

                     request.domain = common::domain::identity();
                     request.process = common::process::handle();
                     request.queues.push_back(  message.name);

                     if( local::optional::send( common::communication::ipc::gateway::manager::optional::device(), std::move( request)))
                     {
                        m_state.pending.push_back( std::move( message));

                        //
                        // We don't send reply, we'll do it when we get the reply from the gateway.
                        //
                        send_reply.release();
                     }
                  }
               }

            } // lookup

            namespace connect
            {
               void Request::operator () ( message_type& message)
               {

                  for( auto&& queue : message.queues)
                  {
                     auto& instances = m_state.queues[ queue.name];

                     instances.emplace_back( message.process, queue.id);

                     common::range::stable_sort( instances);
                  }

                  auto found = common::range::find( m_state.groups, message.process);

                  if( found)
                  {
                     found->connected = true;
                  }
                  else
                  {
                     //
                     // We add the group
                     //
                     m_state.groups.emplace_back( "", message.process);
                  }

               }
            } // connect


            namespace domain
            {
               void Advertise::operator () ( message_type& message)
               {
                  Trace trace{ "handle::domain::Advertise"};

                  log << "message: " << message << '\n';

                  m_state.update( message);

                  for( const auto& queue : message.queues)
                  {


                  }
               }


               namespace discover
               {
                  void Request::operator () ( message_type& message)
                  {
                     Trace trace{ "handle::domain::discover::Request"};

                     auto reply = common::message::reverse::type( message);

                     reply.domain = common::domain::identity();

                     for( auto& queue : message.queues)
                     {
                        if( common::range::find( m_state.queues, queue))
                        {
                           reply.queues.emplace_back( queue);
                        }
                     }

                     ipc::device().blocking_send( message.process.queue, reply);
                  }

                  void Reply::operator () ( message_type& message)
                  {
                     Trace trace{ "handle::domain::discover::Reply"};

                     //
                     // outbound has already advertised the queues (if any), so we have that handled
                     // check if there are any pending lookups for this reply
                     //

                     auto found = common::range::find_if( m_state.pending, [&]( const common::message::queue::lookup::Request& r){
                        return r.correlation == message.correlation;
                     });

                     if( found)
                     {
                        auto request = std::move( *found);
                        m_state.pending.erase( std::begin( found));

                        auto reply = common::message::reverse::type( *found);

                        auto found_queue = common::range::find( m_state.queues, request.name);

                        if( found_queue && ! found_queue->second.empty())
                        {
                           auto& queue = found_queue->second.front();
                           reply.process = queue.process;
                           reply.queue = queue.queue;
                        }

                        local::reply( request.process.queue, reply);
                     }
                     else
                     {
                        log << "no pending was found for discovery reply: " << message << '\n';
                     }
                  }
               } // discover
            } // domain
         } // handle
      } // broker
   } // queue
} // casual

