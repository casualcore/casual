//!
//! handle.cpp
//!
//! Created on: Jun 20, 2014
//!     Author: Lazan
//!

#include "queue/broker/handle.h"

#include "common/log.h"
#include "common/internal/log.h"
#include "common/trace.h"
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
                        common::message::process::termination::Event event{ exit};
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
                  common::Trace trace{ "handle::process::Exit", common::log::internal::queue};

                  {
                     auto found = common::range::find_if( m_state.groups, [=]( const State::Group& g){
                        return g.process.pid == exit.pid;
                     });

                     if( found)
                     {
                        m_state.groups.erase( std::begin( found));
                     }
                     else
                     {
                        // error?
                     }
                  }

                  //
                  // Remove all queues for the group
                  //
                  {

                     auto predicate = [=]( decltype( *m_state.queues.begin())& value){
                        return value.second.process.pid == exit.pid;
                     };

                     auto range = common::range::make( m_state.queues);

                     while( range)
                     {
                        range = common::range::find_if( range, predicate);

                        if( range)
                        {
                           m_state.queues.erase( std::begin( range));
                           range = common::range::make( m_state.queues);
                        }
                     }
                  }
                  //
                  // Invalidate xa-requests
                  //
                  {
                     for( auto& corr : m_state.correlation)
                     {
                        for( auto& reqeust : corr.second.requests)
                        {
                           if( reqeust.group.pid == exit.pid && reqeust.stage <= State::Correlation::Stage::pending)
                           {
                              reqeust.stage = State::Correlation::Stage::error;
                           }
                        }
                     }
                  }
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
                  common::Trace trace{ "handle::lookup::Request", common::log::internal::queue};

                  auto found =  common::range::find( m_state.queues, message.name);

                  if( found)
                  {
                     ipc::device().blocking_send( message.process.queue, found->second);
                  }
                  else
                  {
                     static common::message::queue::lookup::Reply reply;
                     ipc::device().blocking_send( message.process.queue, reply);
                  }
               }

            } // lookup

            namespace connect
            {
               void Request::operator () ( message_type& message)
               {

                  for( auto&& queue : message.queues)
                  {
                     if( ! m_state.queues.emplace( queue.name, common::message::queue::lookup::Reply{ message.process, queue.id}).second)
                     {
                        common::log::error << "multiple instances of queue: " << queue.name << " - action: keeping the first one" << std::endl;
                     }
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

            namespace group
            {
               void Involved::operator () ( message_type& message)
               {
                  auto& involved = m_state.involved[ message.trid];

                  //
                  // Check if we got the involvement of the group already.
                  //
                  auto found = common::range::find( involved, message.process);

                  if( ! found)
                  {
                     involved.emplace_back( message.process);
                  }
               }
            } // group


            namespace transaction
            {

               template< typename message_type>
               void request( State& state, message_type& message)
               {
                  auto found = common::range::find( state.involved, message.trid);

                  auto sendError = [&]( int error_state){
                     auto reply = common::message::reverse::type( message);
                     reply.state = error_state;
                     reply.trid = message.trid;
                     reply.resource = message.resource;
                     reply.process = common::process::handle();

                     ipc::device().blocking_send( message.process.queue, reply);
                  };

                  if( found)
                  {
                     try
                     {
                        //
                        // There are involved groups, send commit request to them...
                        //
                        auto request = message;
                        request.process = common::process::handle();
                        local::send( state, found->second, request);


                        //
                        // Make sure we correlate the coming replies.
                        //
                        common::log::internal::queue << "forward request to groups - correlate response to: " << message.process << "\n";

                        state.correlation.emplace(
                              std::piecewise_construct,
                              std::forward_as_tuple( std::move( found->first)),
                              std::forward_as_tuple( message.process, message.correlation, std::move( found->second)));

                        state.involved.erase( std::begin( found));

                     }
                     catch( ...)
                     {
                        common::error::handler();
                        sendError( XAER_RMFAIL);
                     }
                  }
                  else
                  {
                     common::log::internal::queue << "XAER_NOTA - trid: " << message.trid << " could not be found\n";
                     sendError( XAER_NOTA);
                  }

               }

               template< typename message_type>
               void reply( State& state, message_type&& message)
               {
                  auto found = common::range::find( state.correlation, message.trid);

                  if( ! found)
                  {
                     common::log::error << "failed to correlate reply - trid: " << message.trid << " process: " << message.process << " - action: discard\n";
                     return;
                  }

                  if( message.state == XA_OK)
                  {
                     found->second.stage( message.process, State::Correlation::Stage::replied);
                  }
                  else
                  {
                     found->second.stage( message.process, State::Correlation::Stage::error);
                  }

                  if( found->second.replied())
                  {
                     //
                     // All groups has responded, reply to RM-proxy
                     //
                     message_type reply( message);
                     reply.process = common::process::handle();
                     reply.correlation = found->second.reply_correlation;
                     reply.state = found->second.stage() == State::Correlation::Stage::replied ? XA_OK : XAER_RMFAIL;

                     common::log::internal::queue << "all groups has responded - send reply to RM: " << found->second.caller << std::endl;

                     ipc::device().blocking_send( found->second.caller.queue, reply);

                     //
                     // We're done with the correlation.
                     //
                     state.correlation.erase( std::begin( found));
                  }

               }

               namespace commit
               {
                  void Request::operator () ( message_type& message)
                  {

                     request( m_state, message);

                  }

                  void Reply::operator () ( message_type& message)
                  {
                     reply( m_state, message);
                  }

               } // commit

               namespace rollback
               {
                  void Request::operator () ( message_type& message)
                  {
                     request( m_state, message);

                  }

                  void Reply::operator () ( message_type& message)
                  {
                     reply( m_state, message);
                  }

               } // commit
            } // transaction

         } // handle
      } // broker
   } // queue
} // casual
