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
                              return ! send( g.queue_id, message);
                           });

                     //
                     // Block for the busy ones, if any
                     //
                     queue::blocking::Send blocking_send{ state};

                     for( auto&& group : busy)
                     {
                        blocking_send( group.queue_id, message);
                     }

                  }

               } // <unnamed>
            } // local


            namespace peek
            {
               namespace queue
               {
                  void Request::dispatch( message_type& message)
                  {
                     auto found =  common::range::find( m_state.queues, message.qname);

                     if( found)
                     {
                        //
                        // Forward to group
                        //
                        message.qid = found->second.queue;
                        broker::queue::blocking::Writer write{ found->second.server.queue_id, m_state};
                        write( found->second);
                     }
                     else
                     {
                        // TODO: error?

                        common::message::queue::information::queue::Reply reply;
                        broker::queue::blocking::Writer write{ message.server.queue_id, m_state};
                        write( reply);
                     }
                  }
               } // queue

            } // peek

            namespace lookup
            {

               void Request::dispatch( message_type& message)
               {
                  queue::blocking::Writer write{ message.server.queue_id, m_state};

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
               void Request::dispatch( message_type& message)
               {

                  for( auto&& queue : message.queues)
                  {
                     if( ! m_state.queues.emplace( queue.name, common::message::queue::lookup::Reply{ message.server, queue.id}).second)
                     {
                        common::log::error << "multiple instances of queue: " << queue.name << " - action: keeping the first one" << std::endl;
                     }
                  }
               }
            } // connect

            namespace group
            {
               void Involved::dispatch( message_type& message)
               {
                  auto& involved = m_state.involved[ message.xid.xid];

                  //
                  // Check if we got the involvement of the group already.
                  //
                  auto found = common::range::find( involved, message.server);

                  if( ! found)
                  {
                     involved.emplace_back( message.server);
                  }
               }
            } // group


            namespace transaction
            {

               template< typename message_type>
               void request( State& state, message_type& message)
               {
                  auto found = common::range::find( state.involved, message.xid);

                  auto sendError = [&](){
                     common::message::transaction::resource::commit::Reply reply;
                     reply.state = XAER_RMFAIL;
                     reply.xid = message.xid;
                     reply.resource = message.resource;
                     reply.id = common::message::server::Id::current();
                     queue::blocking::Writer send{ message.id.queue_id, state};
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
                        request.id = common::message::server::Id::current();
                        local::send( state, found->second, request);


                        //
                        // Make sure we correlate the coming replies.
                        //
                        common::log::internal::queue << "forward request to groups - correlate response to: " << message.id << "\n";

                        state.correlation.emplace(
                              std::piecewise_construct,
                              std::forward_as_tuple( std::move( found->first)),
                              std::forward_as_tuple( message.id, std::move( found->second)));

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
                     common::log::internal::queue << "request - xid: " << message.xid << " could not be found - action: XAER_RMFAIL" << std::endl;
                     sendError();
                  }

               }

               template< typename message_type>
               void reply( State& state, message_type&& message)
               {
                  auto found = common::range::find( state.correlation, message.xid);

                  if( ! found)
                  {
                     common::log::error << "failed to correlate reply - xid: " << message.xid << " process: " << message.id << " - action: discard\n";
                     return;
                  }

                  if( message.state == XA_OK)
                  {
                     found->second.state( message.id, State::Correlation::State::replied);
                  }
                  else
                  {
                     found->second.state( message.id, State::Correlation::State::error);
                  }

                  auto groupState = found->second.state();

                  if( groupState >= State::Correlation::State::replied )
                  {
                     //
                     // All groups has responded, reply to RM-proxy
                     //
                     message_type reply( message);
                     reply.id = common::message::server::Id::current();
                     reply.state = groupState == State::Correlation::State::replied ? XA_OK : XAER_RMFAIL;

                     common::log::internal::queue << "all groups has responded - send reply to RM: " << found->second.caller << std::endl;

                     queue::blocking::Writer send{ found->second.caller.queue_id, state};
                     send( reply);
                  }

               }

               namespace commit
               {
                  void Request::dispatch( message_type& message)
                  {

                     request( m_state, message);

                  }

                  void Reply::dispatch( message_type& message)
                  {
                     reply( m_state, message);
                  }

               } // commit

               namespace rollback
               {
                  void Request::dispatch( message_type& message)
                  {
                     request( m_state, message);

                  }

                  void Reply::dispatch( message_type& message)
                  {
                     reply( m_state, message);
                  }

               } // commit
            } // transaction

         } // handle
      } // broker
   } // queue
} // casual
