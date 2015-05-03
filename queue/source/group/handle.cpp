//!
//! handle.cpp
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#include "queue/group/handle.h"

#include "queue/common/environment.h"

#include "common/internal/log.h"
#include "common/error.h"

namespace casual
{

   namespace queue
   {
      namespace group
      {
         namespace queue
         {
            void Policy::apply()
            {
               common::queue::policy::NoAction{}.apply();
            }

         }

         namespace handle
         {

            namespace local
            {
               namespace
               {
                  template< typename M>
                  void involved( State& state, M& message)
                  {
                     common::message::queue::group::Involved involved;
                     involved.process = common::process::handle();
                     involved.trid = message.trid;

                     group::queue::blocking::Writer send{ environment::broker::queue::id(), state};
                     send( involved);
                  }

                  namespace pending
                  {
                     void replies( State& state, const common::transaction::ID& trid)
                     {

                        auto pending = state.pending.commit( trid);

                        for( auto& request : pending.requests)
                        {
                           try
                           {
                              auto& remaining = pending.enqueued[ request.queue];

                              if( remaining > 0)
                              {
                                 if( dequeue::Request{ state}( request))
                                 {
                                    --remaining;
                                 }
                                 else
                                 {
                                    //
                                    // Put it back in pending
                                    //
                                    state.pending.dequeue( request);
                                 }
                              }

                           }
                           catch( const common::exception::queue::Unavailable& exception)
                           {
                              common::log::internal::queue << "ipc-queue unavailable for request: " << request << " - action: ignore\n";
                           }
                        }
                     }
                  } // pending




               } // <unnamed>
            } // local

            namespace information
            {

               namespace queues
               {

                  void Request::operator () ( message_type& message)
                  {
                     common::message::queue::information::queues::Reply reply;
                     reply.correlation = message.correlation;
                     reply.process = common::process::handle();
                     reply.queues = m_state.queuebase.queues();

                     queue::blocking::Writer send{ message.process.queue, m_state};
                     send( reply);
                  }
               } // queues

               namespace messages
               {

                  void Request::operator () ( message_type& message)
                  {
                     common::message::queue::information::messages::Reply reply;
                     reply.correlation = message.correlation;
                     reply.process = common::process::handle();
                     reply.messages = m_state.queuebase.messages( message.qid);

                     queue::blocking::Writer send{ message.process.queue, m_state};
                     send( reply);
                  }

               } // messages
            } // information


            namespace enqueue
            {
               void Request::operator () ( message_type& message)
               {
                  try
                  {
                     auto reply = m_state.queuebase.enqueue( message);
                     reply.correlation = message.correlation;

                     m_state.pending.enqueue( message.trid, message.queue);

                     if( message.trid)
                     {
                        local::involved( m_state, message);

                        queue::blocking::Send send{ m_state};
                        send( message.process.queue, reply);
                     }
                     else
                     {
                        //
                        // enqueue is not in transaction, we guarantee atomic enqueue so
                        // we send reply when whe're in persistent state
                        //
                        m_state.persist( std::move( reply), { message.process.queue});

                        //
                        // Check if there are any pending request for the current queue (and selector).
                        // This could result in the message is dequeued before (persistent) reply to the caller
                        //
                        // We have to do it now, since it won't be any commits...
                        //
                        local::pending::replies( m_state, message.trid);
                     }

                  }
                  catch( const sql::database::exception::Base& exception)
                  {
                     common::log::error << exception.what() << std::endl;
                  }
               }

            } // enqueue

            namespace dequeue
            {
               bool Request::operator () ( message_type& message)
               {
                  try
                  {
                     auto reply = m_state.queuebase.dequeue( message);

                     if( message.block && reply.message.empty())
                     {
                        m_state.pending.dequeue( message);
                     }
                     else
                     {
                        reply.correlation = message.correlation;

                        //
                        // We don't need to be involved in transaction if
                        // have't consumed anything or if we're not in a transaction
                        //
                        if( ! reply.message.empty() && message.trid)
                        {
                           local::involved( m_state, message);
                        }

                        queue::blocking::Send send{ m_state};
                        send( message.process.queue, reply);

                        return true;
                     }
                  }
                  catch( const sql::database::exception::Base& exception)
                  {
                     common::log::error << exception.what() << std::endl;
                  }
                  return false;
               }

            } // dequeue

            namespace transaction
            {
               namespace commit
               {
                  void Request::operator () ( message_type& message)
                  {
                     common::message::transaction::resource::commit::Reply reply;
                     reply.correlation = message.correlation;
                     reply.process = common::process::handle();
                     reply.trid = message.trid;
                     reply.state = XA_OK;

                     try
                     {
                        m_state.queuebase.commit( message.trid);
                        common::log::internal::transaction << "committed trid: " << message.trid << " - number of messages: " << m_state.queuebase.affected() << std::endl;

                        //
                        // Will try to dequeue pending requests
                        //
                        local::pending::replies( m_state, message.trid);
                     }
                     catch( ...)
                     {
                        common::error::handler();
                        reply.state = XAER_RMFAIL;
                     }

                     m_state.persist( std::move( reply), { message.process.queue});
                  }
               }

               namespace rollback
               {
                  void Request::operator () ( message_type& message)
                  {
                     common::message::transaction::resource::rollback::Reply reply;
                     reply.correlation = message.correlation;
                     reply.process = common::process::handle();
                     reply.trid = message.trid;
                     reply.state = XA_OK;

                     try
                     {
                        m_state.queuebase.rollback( message.trid);
                        common::log::internal::transaction << "rollback trid: " << message.trid << " - number of messages: " << m_state.queuebase.affected() << std::endl;

                        //
                        // Removes any associated enqueues with this trid
                        //
                        m_state.pending.rollback( message.trid);
                     }
                     catch( ...)
                     {
                        common::error::handler();
                        reply.state = XAER_RMFAIL;
                     }

                     m_state.persist( std::move( reply), { message.process.queue});
                  }
               }
            }



         } // handle
      } // server
   } // queue
} // casual

