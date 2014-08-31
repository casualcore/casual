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

                     queue::blocking::Writer send{ message.id.queue_id, m_state};
                     send( reply);
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

                     queue::blocking::Writer send{ message.id.queue_id, m_state};
                     send( reply);
                  }
               }
            }



         } // handle
      } // server
   } // queue
} // casual

