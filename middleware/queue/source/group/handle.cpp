//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/group/handle.h"
#include "queue/common/log.h"

#include "common/message/handle.h"
#include "common/event/listen.h"
#include "common/exception/handle.h"
#include "common/execute.h"

#include "common/communication/instance.h"

#include "domain/pending/send/message.h"

namespace casual
{
   using namespace common;

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
                  namespace transaction
                  {
                     template< typename M>
                     void involved( State& state, M& message)
                     {
                        if( ! common::algorithm::find( state.involved, message.trid))
                        {
                           auto involved = common::message::transaction::resource::external::involved::create( message);

                           common::communication::ipc::blocking::send(
                                 common::communication::instance::outbound::transaction::manager::device(),
                                 involved);
                        }
                     }

                     template< typename M>
                     void done( State& state, M& message)
                     {
                        common::algorithm::trim( state.involved, common::algorithm::remove( state.involved, message.trid));
                     }


                  } // transaction


                  namespace pending
                  {
                     void dequeue( State& state)
                     {
                        Trace trace{ "queue::handle::local::pending::replies"};

                        
                        if( state.queuebase.has_pending())
                        {
                           auto pending = state.queuebase.pending();

                           common::log::line( verbose::log, "pending: ", pending);

                           auto failed = std::get< 1>( common::algorithm::partition( pending, [&]( auto& request)
                           {
                              return dequeue::request( state, request);
                           }));

                           if( failed)
                           {
                              common::log::line( common::log::category::error, "failed to handle all pending dequeues");
                              common::log::line( common::log::category::verbose::error, "failed: ", failed);
                           }
                        }
                     }
                  } // pending

                  namespace ipc
                  {
                     namespace blocking
                     {
                        template< typename D, typename M>
                        common::Uuid send( D&& device, M&& message)
                        {
                           try
                           {
                              return common::communication::ipc::blocking::send( device, std::forward< M>( message));
                           }
                           catch( const common::exception::system::communication::Unavailable&)
                           {
                              return common::uuid::empty();
                           }
                        }
                     } // blocking


                  } // ipc

                  namespace persistent
                  {
                     void send( State& state)
                     {
                        auto send = []( auto& pending)
                        {
                           if( ! common::message::pending::non::blocking::send( pending))
                              casual::domain::pending::send::message( pending);
                        };

                        // persist
                        state.persist();

                        auto pending = std::exchange( state.pending.persistent, {});

                        algorithm::for_each( pending, send);
                     }
                  } // persistent

                  namespace possible
                  {
                     template< typename M> 
                     void persist( State& state, const process::Handle& destination, M&& message)
                     {
                        state.pending.persist( std::forward< M>( message), destination);

                        if( state.pending.persistent.size() >= common::platform::batch::queue::persitent)
                        {
                           local::persistent::send( state);
                        }
                     }
                  } // possible

               } // <unnamed>
            } // local


            void shutdown( State& state)
            {
               Trace trace{ "queue::handle::shutdown"};

               algorithm::move( state.queuebase.pending_forget(), state.pending.persistent);

               handle::persistent::send( state);
            }

            namespace persistent
            {
               void send( State& state)
               {
                  if( ! state.pending.persistent.empty())
                     local::persistent::send( state);
               }
            } // persistent

            namespace dead
            {
               void Process::operator() ( const common::message::event::process::Exit& message)
               {
                  Trace trace{ "queue::handle::dead::Process"};

                  // We check and do some clean up, if the dead process has any pending replies.
                  m_state.queuebase.pending_erase( message.state.pid);
               }

            } // dead

            namespace information
            {

               namespace queues
               {

                  void Request::operator () ( message_type& message)
                  {
                     Trace trace{ "queue::handle::information::queues::request"};

                     common::message::queue::information::queues::Reply reply;
                     reply.correlation = message.correlation;
                     reply.process = common::process::handle();
                     reply.queues = m_state.queuebase.queues();

                     common::communication::ipc::blocking::send( message.process.ipc, reply);
                  }
               } // queues

               namespace messages
               {

                  void Request::operator () ( message_type& message)
                  {
                     Trace trace{ "queue::handle::information::messages::request"};

                     common::message::queue::information::messages::Reply reply;
                     reply.correlation = message.correlation;
                     reply.process = common::process::handle();
                     reply.messages = m_state.queuebase.messages( message.qid);

                     common::communication::ipc::blocking::send( message.process.ipc, reply);
                  }

               } // messages
            } // information


            namespace enqueue
            {
               void Request::operator () ( message_type& message)
               {
                  Trace trace{ "queue::handle::enqueue::Request"};

                  try
                  {
                     // Make sure we've got the quid.
                     message.queue =  m_state.queuebase.quid( message);

                     auto reply = m_state.queuebase.enqueue( message);

                     if( message.trid)
                     {
                        local::transaction::involved( m_state, message);

                        local::ipc::blocking::send( message.process.ipc, reply);
                     }
                     else
                     {
                        // enqueue is not in transaction, we guarantee atomic enqueue so
                        // we send reply when we're in persistent state
                        local::possible::persist( m_state, message.process, std::move( reply));

                        // Check if there are any pending request for the current queue (and selector).
                        // This could result in the message is dequeued before (persistent) reply to the caller
                        //
                        // We have to do it now, since it won't be any commits...
                        local::pending::dequeue( m_state);
                     }

                  }
                  catch( const sql::database::exception::Base& exception)
                  {
                     log::line( log::category::error, exception.what());
                  }
               }

            } // enqueue

            namespace dequeue
            {

               bool request( State& state, Request::message_type& message)
               {
                  Trace trace{ "queue::handle::dequeue::request"};

                  try
                  {
                     auto reply = state.queuebase.dequeue( message);

                     if( ! reply.message.empty() || ! message.block)
                     {
                        reply.correlation = message.correlation;

                        // We don't need to be involved in transaction if
                        // haven't consumed anything or if we're not in a transaction
                        if( ! reply.message.empty() && message.trid)
                        {
                           local::transaction::involved( state, message);
                        }

                        local::ipc::blocking::send( message.process.ipc, reply);

                        return true;
                     }
                  }
                  catch( const sql::database::exception::Base& exception)
                  {
                     log::line( log::category::error, exception.what());
                  }
                  return false;
               }


               void Request::operator () ( message_type& message)
               {
                  Trace trace{ "queue::handle::dequeue::Request"};

                  // Make sure we've got the quid.
                  message.queue =  m_state.queuebase.quid( message);

                  request( m_state, message);
               }

               namespace forget
               {
                  void Request::operator () ( message_type& message)
                  {
                     Trace trace{ "queue::handle::dequeue::forget::Request"};

                     try
                     {

                        auto reply = m_state.queuebase.pending_forget( message);

                        common::communication::ipc::blocking::send( message.process.ipc, reply);
                     }
                     catch( ...)
                     {
                        common::exception::handle();
                     }
                  }

               } // forget

            } // dequeue

            namespace peek
            {
               namespace information
               {
                  void Request::operator () ( message_type& message)
                  {
                     Trace trace{ "queue::handle::peek::information::Request"};

                     local::ipc::blocking::send( message.process.ipc, m_state.queuebase.peek( message));

                  }

               } // information

               namespace messages
               {
                  void Request::operator () ( message_type& message)
                  {
                     Trace trace{ "queue::handle::peek::information::Request"};

                     local::ipc::blocking::send( message.process.ipc, m_state.queuebase.peek( message));
                  }
               } // messages

            } // peek

            namespace transaction
            {
               namespace commit
               {
                  void Request::operator () ( message_type& message)
                  {
                     Trace trace{ "queue::handle::transaction::commit::Request"};

                     local::transaction::done( m_state, message);

                     auto reply = common::message::reverse::type( message);
                     reply.process = common::process::handle();
                     reply.resource = message.resource;
                     reply.trid = message.trid;
                     reply.state = common::code::xa::ok;

                     try
                     {
                        m_state.queuebase.commit( message.trid);
                        log::line( log::category::transaction, "committed trid: ", message.trid, " - number of messages: ", m_state.queuebase.affected());

                        // Will try to dequeue pending requests
                        local::pending::dequeue( m_state);
                     }
                     catch( ...)
                     {
                        common::exception::handle();
                        reply.state = common::code::xa::resource_fail;
                     }

                     local::possible::persist( m_state, message.process, std::move( reply));
                  }
               }

               namespace prepare
               {

                  void Request::operator () ( message_type& message)
                  {
                     Trace trace{ "queue::handle::transaction::prepare::Request"};

                     auto reply = common::message::reverse::type( message);
                     reply.process = common::process::handle();
                     reply.resource = message.resource;
                     reply.trid = message.trid;
                     reply.state = common::code::xa::ok;

                     local::ipc::blocking::send( common::communication::instance::outbound::transaction::manager::device(), reply);
                  }
               }

               namespace rollback
               {
                  void Request::operator () ( message_type& message)
                  {
                     Trace trace{ "queue::handle::transaction::rollback::Request"};

                     local::transaction::done( m_state, message);

                     auto reply = common::message::reverse::type( message);
                     reply.process = common::process::handle();
                     reply.resource = message.resource;
                     reply.trid = message.trid;
                     reply.state = common::code::xa::ok;

                     try
                     {
                        m_state.queuebase.rollback( message.trid);
                        common::log::line( common::log::category::transaction, "rollback trid: ", message.trid, 
                           " - number of messages: ", m_state.queuebase.affected());
                     }
                     catch( ...)
                     {
                        common::exception::handle();
                        reply.state = common::code::xa::resource_fail;
                     }

                     local::possible::persist( m_state, message.process, std::move( reply));
                  }
               }
            } // transaction

            namespace restore
            {
               void Request::operator () ( message_type& message)
               {
                  Trace trace{ "queue::handle::restore::Request"};

                  auto reply = common::message::reverse::type( message);

                  auto send_reply = common::execute::scope( [&](){
                     local::ipc::blocking::send( message.process.ipc, reply);
                  });

                  reply.affected = common::algorithm::transform( message.queues, [&]( auto queue){
                     common::message::queue::restore::Reply::Affected result;

                     result.queue = queue;
                     result.restored = m_state.queuebase.restore( queue);
                     return result;
                  });

                  // Make sure pending get a crack at the restored messages.
                  local::pending::dequeue( m_state);
               }

            } // restore

         } // handle

         handle::dispatch_type handler( State& state)
         {
            return {
               common::event::listener( handle::dead::Process{ state}),
               handle::enqueue::Request{ state},
               handle::dequeue::Request{ state},
               handle::dequeue::forget::Request{ state},
               handle::transaction::commit::Request{ state},
               handle::transaction::prepare::Request{ state},
               handle::transaction::rollback::Request{ state},
               handle::information::queues::Request{ state},
               handle::information::messages::Request{ state},
               handle::peek::information::Request{ state},
               handle::peek::messages::Request{ state},
               handle::restore::Request{ state},
               common::message::handle::Shutdown{},
            };
         }

      } // server
   } // queue
} // casual

