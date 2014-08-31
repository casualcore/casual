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





namespace casual
{
   namespace queue
   {
      namespace broker
      {
         namespace queue
         {
            void Policy::apply()
            {

            }

         }

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
                              queue::non_blocking::Writer send{ g.id.queue_id, state};
                              return ! send( message);
                           });

                     //
                     // Block for the busy ones, if any
                     //
                     for( auto&& group : busy)
                     {
                        queue::blocking::Writer send{ group.id.queue_id, state};
                        send( message);
                     }

                  }

               } // <unnamed>
            } // local

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
                     static const common::message::queue::lookup::Reply reply;
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
                  auto found = common::range::find_if( involved,
                        [&]( const State::Group& g){ return g.id.pid == message.server.pid;});

                  if( ! found)
                  {
                     involved.emplace_back( message.server);
                  }
               }
            } // group


            namespace transaction
            {

               template< typename message_type>
               void request( State& state, message_type&& message)
               {
                  auto found = common::range::find( state.involved, message.xid);

                  if( found)
                  {
                     try
                     {
                        //
                        // There are involved groups, send commit request to them...
                        //
                        message_type request( message);
                        request.id = common::message::server::Id::current();
                        local::send( state, found->second, request);


                     }
                     catch( ...)
                     {
                        common::error::handler();
                        common::message::transaction::resource::commit::Reply reply;
                        reply.state = XAER_RMFAIL;
                        reply.xid = message.xid;
                        reply.id = common::message::server::Id::current();
                        queue::blocking::Writer send{ message.id.queue_id, state};
                        send( message);
                     }

                     //
                     // Make sure we correlate the coming replies.
                     //
                     auto removed = state.involved.erase( found.first);

                     state.correlation.emplace(
                           std::piecewise_construct,
                           std::forward_as_tuple( std::move( removed->first)),
                           std::forward_as_tuple( message.id, std::move( removed->second)));
                  }
                  else
                  {
                     common::log::internal::transaction << "request - xid: " << message.xid << " could not be found - action: discard" << std::endl;
                  }

               }

               template< typename message_type>
               void reply( State& state, message_type&& message)
               {
                  auto found = common::range::find( state.correlation, message.xid);

                  if( found)
                  {
                     if( message.state == XA_OK)
                     {
                        found->second.state( message.id, State::Correlation::State::replied);
                     }
                     else
                     {
                        found->second.state( message.id, State::Correlation::State::error);
                     }
                  }

                  auto groupState = found->second.state();

                  if( groupState >= State::Correlation::State::replied )
                  {
                     //
                     // All groups has responded, reply to RM-proxyn
                     //
                     message_type reply( message);
                     reply.id = common::message::server::Id::current();
                     reply.state = groupState == State::Correlation::State::replied ? XA_OK : XAER_RMFAIL;

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
