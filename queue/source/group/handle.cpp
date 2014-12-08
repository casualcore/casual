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


                  template< typename M>
                  void callback( State& state, M& message)
                  {
                     auto found = state.callbacks.find( message.queue);

                     if( found != std::end( state.callbacks))
                     {
                        auto& callbacks = found->second;

                        common::message::queue::dequeue::callback::Reply reply{ common::process::handle(), message.queue};
                        group::queue::non_blocking::Send send{ state};

                        auto sender = std::bind( std::ref( send), std::placeholders::_1, std::ref( reply));

                        auto sent = common::range::find_if( callbacks, sender);

                        if( sent)
                        {
                           callbacks.erase( sent.first);
                        }
                     }
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
                     reply.process = common::process::handle();
                     reply.queues = m_state.queuebase.queues();

                     queue::blocking::Writer send{ message.process.queue, m_state};
                     send( reply);
                  }
               } // queues

               namespace messages
               {

                  void Request::dispatch( message_type& message)
                  {
                     common::message::queue::information::queue::Reply reply;
                     reply.process = common::process::handle();
                     reply.messages = m_state.queuebase.messages( message.qid);

                     queue::blocking::Writer send{ message.process.queue, m_state};
                     send( reply);
                  }

               } // messages

            } // information



            namespace enqueue
            {
               void Request::dispatch( message_type& message)
               {
                  queue::blocking::Writer send{ message.process.queue, m_state};

                  try
                  {
                     auto reply = m_state.queuebase.enqueue( message);
                     local::involved( m_state, message);
                     send( reply);

                     local::callback( m_state, message);

                  }
                  catch( const sql::database::exception::Base& exception)
                  {
                     common::log::error << exception.what() << std::endl;
                  }
               }

            } // enqueue

            namespace dequeue
            {
               void Request::dispatch( message_type& message)
               {
                  do_dispatch( message);
               }

               template< typename M>
               bool Request::do_dispatch( M& message)
               {
                  queue::blocking::Writer send{ message.process.queue, m_state};

                  try
                  {
                     auto reply = m_state.queuebase.dequeue( message);
                     reply.correlation = message.correlation;
                     local::involved( m_state, message);
                     send( reply);


                     return ! reply.message.empty();
                  }
                  catch( const sql::database::exception::Base& exception)
                  {
                     common::log::error << exception.what() << std::endl;
                  }
                  return true;
               }

               namespace callback
               {
                  void Request::dispatch( message_type& message)
                  {
                     if( ! dequeue::Request::do_dispatch( message))
                     {
                        //
                        // No messages in queue, set up a call-back, that will be called
                        // when a message is enqueued to the queue
                        //
                        m_state.callbacks[ message.queue].push_back( message.process.queue);

                     }

                  }
               } // callback

            } // dequeue

            namespace transaction
            {
               namespace commit
               {
                  void Request::dispatch( message_type& message)
                  {
                     common::message::transaction::resource::commit::Reply reply;
                     reply.process = common::process::handle();
                     reply.trid = message.trid;
                     reply.state = XA_OK;

                     try
                     {
                        m_state.queuebase.commit( message.trid);
                        common::log::internal::transaction << "committed trid: " << message.trid << " - number of messages: " << m_state.queuebase.affected() << std::endl;
                     }
                     catch( ...)
                     {
                        common::error::handler();
                        reply.state = XAER_RMFAIL;
                     }

                     m_state.persist( std::move( reply), message.process.queue);
                  }
               }

               namespace rollback
               {
                  void Request::dispatch( message_type& message)
                  {
                     common::message::transaction::resource::rollback::Reply reply;
                     reply.process = common::process::handle();
                     reply.trid = message.trid;
                     reply.state = XA_OK;

                     try
                     {
                        m_state.queuebase.rollback( message.trid);
                        common::log::internal::transaction << "rollback trid: " << message.trid << " - number of messages: " << m_state.queuebase.affected() << std::endl;
                     }
                     catch( ...)
                     {
                        common::error::handler();
                        reply.state = XAER_RMFAIL;
                     }

                     m_state.persist( std::move( reply), message.process.queue);
                  }
               }
            }



         } // handle
      } // server
   } // queue
} // casual

