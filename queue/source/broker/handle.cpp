//!
//! handle.cpp
//!
//! Created on: Jun 20, 2014
//!     Author: Lazan
//!

#include "queue/broker/handle.h"

#include "common/log.h"
#include "common/internal/log.h"
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

                     queue::non_blocking::Send send{ state};

                     auto busy = common::range::partition( groups, [&]( group_type& g)
                           {
                              return ! send( g.queue, message);
                           });

                     //
                     // Block for the busy ones, if any
                     //
                     queue::blocking::Send blocking_send{ state};

                     for( auto&& group : std::get< 0>( busy))
                     {
                        blocking_send( group.queue, message);
                     }

                  }

               } // <unnamed>
            } // local


            namespace shutdown
            {
               void Request::operator () ( message_type& message)
               {
                  std::vector< common::process::Handle> groups;
                  common::range::transform( m_state.groups, groups, std::mem_fn( &broker::State::Group::process));

                  for( auto pid : common::server::lifetime::soft::shutdown( groups, std::chrono::seconds( 1)))
                  {
                     m_state.removeProcess( pid);
                  }

                  throw common::exception::Shutdown{ "shutting down", __FILE__, __LINE__};
               }

            } // shutdown

            namespace lookup
            {

               void Request::operator () ( message_type& message)
               {
                  queue::blocking::Writer write{ message.process.queue, m_state};

                  auto found =  common::range::find( m_state.queues, message.name);

                  if( found)
                  {
                     write( found->second);
                  }
                  else
                  {
                     static common::message::queue::lookup::Reply reply;
                     write( reply);
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

                  auto sendError = [&](){
                     common::message::transaction::resource::commit::Reply reply;
                     reply.state = XAER_RMFAIL;
                     reply.trid = message.trid;
                     reply.resource = message.resource;
                     reply.process = common::process::handle();
                     queue::blocking::Writer send{ message.process.queue, state};
                     send( message);
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
                              std::forward_as_tuple( message.process, std::move( found->second)));

                        state.involved.erase( found.first);

                     }
                     catch( ...)
                     {
                        common::error::handler();
                        sendError();
                     }
                  }
                  else
                  {
                     common::log::internal::queue << "request - trid: " << message.trid << " could not be found - action: XAER_RMFAIL" << std::endl;
                     sendError();
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
                     found->second.state( message.process, State::Correlation::State::replied);
                  }
                  else
                  {
                     found->second.state( message.process, State::Correlation::State::error);
                  }

                  auto groupState = found->second.state();

                  if( groupState >= State::Correlation::State::replied )
                  {
                     //
                     // All groups has responded, reply to RM-proxy
                     //
                     message_type reply( message);
                     reply.process = common::process::handle();
                     reply.state = groupState == State::Correlation::State::replied ? XA_OK : XAER_RMFAIL;

                     common::log::internal::queue << "all groups has responded - send reply to RM: " << found->second.caller << std::endl;

                     queue::blocking::Writer send{ found->second.caller.queue, state};
                     send( reply);
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
