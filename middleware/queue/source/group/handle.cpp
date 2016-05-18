//!
//! handle.cpp
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#include "queue/group/handle.h"

#include "queue/common/environment.h"

#include "common/internal/log.h"
#include "common/trace.h"
#include "common/error.h"


// todo: temp
#include <iostream>

namespace casual
{

   namespace queue
   {
      namespace group
      {

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

                     common::communication::ipc::blocking::send( environment::ipc::broker::device(), involved);
                  }

                  namespace pending
                  {
                     void replies( State& state, const common::transaction::ID& trid)
                     {
                        common::trace::Scope trace{ "queue::handle::pending", common::log::internal::queue};


                        auto pending = state.pending.commit( trid);

                        for( auto& request : pending.requests)
                        {
                           try
                           {
                              auto& remaining = pending.enqueued[ request.queue];

                              if( remaining > 0)
                              {
                                 if( dequeue::request( state, request))
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

            namespace dead
            {
               void Process::operator() ( const common::message::domain::process::termination::Event& message)
               {
                  common::trace::Scope trace{ "queue::handle::dead::Process", common::log::internal::queue};

                  //
                  // We check and do some clean up, if the dead process has any pending replies.
                  //
                  m_state.pending.erase( message.death.pid);
               }

            } // dead

            namespace information
            {

               namespace queues
               {

                  void Request::operator () ( message_type& message)
                  {
                     common::trace::Scope trace{ "queue::handle::information::queues::request", common::log::internal::queue};

                     common::message::queue::information::queues::Reply reply;
                     reply.correlation = message.correlation;
                     reply.process = common::process::handle();
                     reply.queues = m_state.queuebase.queues();

                     common::communication::ipc::blocking::send( message.process.queue, reply);
                  }
               } // queues

               namespace messages
               {

                  void Request::operator () ( message_type& message)
                  {
                     common::trace::Scope trace{ "queue::handle::information::messages::request", common::log::internal::queue};

                     common::message::queue::information::messages::Reply reply;
                     reply.correlation = message.correlation;
                     reply.process = common::process::handle();
                     reply.messages = m_state.queuebase.messages( message.qid);

                     common::communication::ipc::blocking::send( message.process.queue, reply);
                  }

               } // messages
            } // information


            namespace enqueue
            {
               void Request::operator () ( message_type& message)
               {
                  common::trace::Scope trace{ "queue::handle::enqueue::request", common::log::internal::queue};

                  try
                  {
                     auto reply = m_state.queuebase.enqueue( message);
                     reply.correlation = message.correlation;

                     m_state.pending.enqueue( message.trid, message.queue);

                     if( message.trid)
                     {
                        local::involved( m_state, message);

                        common::communication::ipc::blocking::send( message.process.queue, reply);
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

               bool request( State& state, Request::message_type& message)
               {
                  common::trace::Scope trace{ "queue::handle::dequeue::request", common::log::internal::queue};

                  try
                  {
                     auto reply = state.queuebase.dequeue( message);

                     if( message.block && reply.message.empty())
                     {
                        state.pending.dequeue( message);
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
                           local::involved( state, message);
                        }

                        common::communication::ipc::blocking::send( message.process.queue, reply);

                        return true;
                     }
                  }
                  catch( const sql::database::exception::Base& exception)
                  {
                     common::log::error << exception.what() << std::endl;
                  }
                  return false;
               }


               void Request::operator () ( message_type& message)
               {
                  request( m_state, message);
               }

               namespace forget
               {
                  void Request::operator () ( message_type& message)
                  {
                     common::trace::Scope trace{ "queue::handle::dequeue::forget::Request", common::log::internal::queue};

                     try
                     {

                        auto reply = m_state.pending.forget( message);

                        common::communication::ipc::blocking::send( message.process.queue, reply);
                     }
                     catch( ...)
                     {
                        common::error::handler();
                     }
                  }

               } // forget

            } // dequeue

            namespace transaction
            {
               namespace commit
               {
                  void Request::operator () ( message_type& message)
                  {
                     common::trace::Scope trace{ "queue::handle::transaction::commit::Request", common::log::internal::queue};

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
                     common::trace::Scope trace{ "queue::handle::transaction::rollback::Request", common::log::internal::queue};

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

