//!
//! handle.cpp
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#include "queue/group/handle.h"

#include "queue/environment.h"

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
                     involved.server = common::message::server::Id::current();
                     involved.xid = message.xid;

                     group::queue::blocking::Writer send{ environment::broker::queue::id(), state};
                     send( involved);
                  }


               } // <unnamed>
            } // local

            namespace information
            {

               namespace queues
               {

                  void Request::dispatch( message_type& message)
                  {
                     common::message::queue::information::queues::Reply reply;
                     reply.server = common::message::server::Id::current();
                     reply.queues = m_state.queuebase.queues();

                     queue::blocking::Writer send{ message.server.queue_id, m_state};
                     send( reply);
                  }
               } // queues

               namespace messages
               {

                  void Request::dispatch( message_type& message)
                  {
                     common::message::queue::information::queue::Reply reply;
                     reply.server = common::message::server::Id::current();
                     reply.messages = m_state.queuebase.messages( message.qid);

                     queue::blocking::Writer send{ message.server.queue_id, m_state};
                     send( reply);
                  }

               } // messages

            } // information



            namespace enqueue
            {
               void Request::dispatch( message_type& message)
               {
                  m_state.queuebase.enqueue( message);
                  local::involved( m_state, message);

               }

            } // enqueue

            namespace dequeue
            {
               void Request::dispatch( message_type& message)
               {
                  auto reply = m_state.queuebase.dequeue( message);
                  queue::blocking::Writer send{ message.server.queue_id, m_state};
                  send( reply);
                  local::involved( m_state, message);
               }
            } // dequeue

            namespace transaction
            {
               namespace commit
               {
                  void Request::dispatch( message_type& message)
                  {
                     common::message::transaction::resource::commit::Reply reply;
                     reply.id = common::message::server::Id::current();
                     reply.xid = message.xid;
                     reply.state = XA_OK;

                     try
                     {
                        m_state.queuebase.commit( message.xid);
                        common::log::internal::transaction << "committed xid: " << message.xid << " - number of messages: " << m_state.queuebase.affected() << std::endl;
                     }
                     catch( ...)
                     {
                        common::error::handler();
                        reply.state = XAER_RMFAIL;
                     }

                     m_state.persist( std::move( reply), message.id.queue_id);
                  }
               }

               namespace rollback
               {
                  void Request::dispatch( message_type& message)
                  {
                     common::message::transaction::resource::rollback::Reply reply;
                     reply.id = common::message::server::Id::current();
                     reply.xid = message.xid;
                     reply.state = XA_OK;

                     try
                     {
                        m_state.queuebase.rollback( message.xid);
                        common::log::internal::transaction << "rollback xid: " << message.xid << " - number of messages: " << m_state.queuebase.affected() << std::endl;
                     }
                     catch( ...)
                     {
                        common::error::handler();
                        reply.state = XAER_RMFAIL;
                     }

                     m_state.persist( std::move( reply), message.id.queue_id);
                  }
               }
            }



         } // handle
      } // server
   } // queue
} // casual

