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

#include "domain/pending/message/send.h"

namespace casual
{
   using namespace common;

   namespace queue
   {
      namespace group
      {
         namespace handle
         {

            namespace ipc
            {
               const common::communication::ipc::Helper& device()
               {
                  static communication::ipc::Helper singleton{ []()
                  {
                     try
                     {
                        throw;
                     }
                     catch( const exception::signal::Timeout&)
                     {
                        // Timeout has occurred, we push the corresponding 
                        // signal to our own "queue", and handle it "later"
                        common::communication::ipc::inbound::device().push( common::message::signal::Timeout{});
                     }
                  }};
                  return singleton;
               }
            } // ipc

            namespace local
            {
               namespace
               {
                  namespace ipc
                  {
                     namespace blocking
                     {
                        template< typename D, typename M>
                        void send( D&& device, M&& message)
                        {
                           try
                           {
                              handle::ipc::device().blocking_send( device, std::forward< M>( message));
                           }
                           catch( const common::exception::system::communication::Unavailable&)
                           {
                              log::line( log, "destination not available: ", device, " - action: ignore");
                              log::line( verbose::log, "dropped message: ", message);
                           }
                        }
                     } // blocking

                     namespace eventually
                     {
                        template< typename M>
                        void send( const process::Handle& destination, M&& message)
                        {
                           try
                           {
                              if( ! handle::ipc::device().non_blocking_send( destination.ipc, message))
                                 casual::domain::pending::message::send( destination, message, handle::ipc::device().error_handler()); 
                           }
                           catch( const common::exception::system::communication::Unavailable&)
                           {
                              log::line( log, "destination not available: ", destination, " - action: ignore");
                              log::line( verbose::log, "dropped message: ", message);
                           }                         
                        }
                     } // eventually

                     namespace pending
                     {
                        void send( common::message::pending::Message& pending)
                        {
                           const auto& error_handler = handle::ipc::device().error_handler();
                           if( ! common::message::pending::non::blocking::send( pending, error_handler))
                              casual::domain::pending::message::send( pending, error_handler);
                        }
                     } // pending

                  } // ipc

                  namespace transaction
                  {
                     template< typename M>
                     void involved( State& state, M& message)
                     {
                        if( ! algorithm::find( state.involved, message.trid))
                        {
                           ipc::blocking::send( 
                              communication::instance::outbound::transaction::manager::device(),
                              common::message::transaction::resource::external::involved::create( message));
                        }
                     }

                     template< typename M>
                     void done( State& state, M& message)
                     {
                        algorithm::trim( state.involved, algorithm::remove( state.involved, message.trid));
                     }

                  } // transaction


                  namespace persistent
                  {
                     template< typename M> 
                     void reply( State& state, const process::Handle& destination, M&& message)
                     {
                        state.pending.reply( std::forward< M>( message), destination);

                        if( state.pending.replies.size() >= common::platform::batch::queue::persistent)
                           handle::persist( state);
                     }

                  } // persistent

                  namespace pending
                  { 
                     void dequeues( State& state)
                     {
                        Trace trace{ "queue::handle::local::pending::dequeues"};

                        if( state.pending.dequeues.empty())
                           return; // nothing to do

                        log::line( verbose::log, "state.pending.dequeues: ", state.pending.dequeues);

                        auto transform_id = []( auto& value){ return value.queue;};
                        
                        // get available queues based on what we've got pending
                        auto available = state.queuebase.available( 
                           algorithm::transform( state.pending.dequeues, transform_id));
                        
                        log::line( verbose::log, "available: ", available);

                        const auto now = platform::time::clock::type::now();

                        auto split = algorithm::partition( available, [&now]( auto& a){ return a.when <= now;});


                        // take care of available
                        {
                           auto passed = std::get< 0>( split);
                           auto requests = state.pending.extract( algorithm::transform( passed, transform_id));
                           auto current = range::make( requests);

                           auto dequeue = handle::dequeue::Request{ state};

                           for( auto& available : passed)
                           {
                              auto partition = algorithm::partition( current, [queue = available.queue]( auto& request)
                              {
                                 return queue == request.queue;
                              });

                              auto reached_message_count = [count = available.count, &dequeue]( auto& message) mutable
                              {  
                                 // we only keep (and don't try to deque) if we "know" 
                                 if( count < 1)
                                    return true;

                                 if( dequeue( message))
                                    --count;

                                 return false;
                              };

                              // we put back all request that is left when we reach the count-limit (mo more messages on the queue)
                              // this is an optimization, since we know how many messages there are on the queue.
                              algorithm::move( 
                                 algorithm::filter( std::get< 0>( partition), reached_message_count),
                                 std::back_inserter( state.pending.dequeues));

                              // continue with the 'complement' of the partition next iteration.
                              current = std::get< 1>( partition);
                           }
                        } 

                        // take care of not available ( yet). We find the earlest and set a timer for that
                        {
                           auto earliest_future = algorithm::min( std::get< 1>( split), []( auto& l, auto& r){ return l.when < r.when;});

                           if( earliest_future)
                              common::signal::timer::set( earliest_future.front().when - now);
                           else 
                              common::signal::timer::unset();
                        }

                        // TODO maintainability: might be to long function - might be able to simplify

                     }
                     
                  } // pending

               } // <unnamed>
            } // local


            void shutdown( State& state)
            {
               Trace trace{ "queue::handle::shutdown"};

               handle::persist( state);

               // send _forget requests_ to pending dequeue requests, if any.
               algorithm::for_each( state.pending.forget(), &local::ipc::pending::send);

            }

            void persist( State& state)
            {
               Trace trace{ "queue::handle::persist"};

               if( state.pending.replies.empty())
                  return; // nothing to do.

               // persist the queuebase
               state.persist();
               
               algorithm::for_each( std::exchange( state.pending.replies, {}), &local::ipc::pending::send);

               // handle pending dequeues, if any.
               local::pending::dequeues( state);
            }


            namespace dead
            {
               void Process::operator() ( const common::message::event::process::Exit& message)
               {
                  Trace trace{ "queue::handle::dead::Process"};

                  // we clear up our own pending state, TM will send us rollback if the
                  // process owned any transactions we have as pending enqueue/dequeue (this 
                  // could have taken place already)
                  m_state.pending.remove( message.state.pid);
               }

            } // dead

            namespace information
            {
               namespace queues
               {
                  void Request::operator () ( common::message::queue::information::queues::Request& message)
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
                  void Request::operator () ( common::message::queue::information::messages::Request& message)
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
               void Request::operator () ( common::message::queue::enqueue::Request& message)
               {
                  Trace trace{ "queue::handle::enqueue::Request"};

                  try
                  {
                     // Make sure we've got the quid.
                     message.queue = m_state.queuebase.id( message);

                     auto reply = m_state.queuebase.enqueue( message);

                     if( message.trid)
                     {
                        // for clarification: TM is guaranteed to consume 
                        // the involved message before caller issue any 
                        // transaction messages of their own (since
                        // we send 'involved' first).
                        local::transaction::involved( m_state, message);
                        local::ipc::blocking::send( message.process.ipc, reply);
                     }
                     else
                     {
                        // enqueue is not in transaction, we guarantee atomic enqueue so
                        // we send reply when we're in persistent state
                        local::persistent::reply( m_state, message.process, std::move( reply));

                        // handle::persist will take care of pending dequeue requests
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
               bool Request::operator () ( common::message::queue::dequeue::Request& message)
               {
                  Trace trace{ "queue::handle::dequeue::Request"};

                  try
                  {
                     // Make sure we've got the quid.
                     message.queue = m_state.queuebase.id( message);

                     return handle( message);
                  }
                  catch( const sql::database::exception::Base& exception)
                  {
                     log::line( log::category::error, exception.what());
                  }
                  return true;
               }

               bool Request::handle( common::message::queue::dequeue::Request& message)
               {
                  Trace trace{ "queue::handle::dequeue::Request::handle"};
                  log::line( verbose::log, "message: ", message);

                  auto now = platform::time::clock::type::now();

                  auto reply = m_state.queuebase.dequeue( message, now);
                  reply.correlation = message.correlation;

                  // make sure we always send reply
                  auto send_reply = execute::scope( [&]()
                  {
                     local::ipc::blocking::send( message.process.ipc, reply);
                  });

                  if( ! reply.message.empty())
                  {
                     // we notify TM if the dequeue is in a transaction
                     if( message.trid)
                        local::transaction::involved( m_state, message);
                  }
                  else if( message.block)
                  {
                     // check if we need to set a timer
                     auto available = m_state.queuebase.available( message.queue);
                     if( available)
                     {
                        auto wanted = available.value() - now;
                        auto current = common::signal::timer::get();
                        log::line( verbose::log, "wanted: ", wanted, ", current: ", current);
                        if( current == common::platform::time::unit::min() || wanted < current)
                           common::signal::timer::set( wanted);
                     }

                     // no message, but caller wants to block
                     m_state.pending.add( std::move( message));

                     // we don't send reply
                     send_reply.release();

                     return false;
                  }
                  return true;
               }

               namespace forget
               {
                  void Request::operator () ( common::message::queue::dequeue::forget::Request& message)
                  {
                     Trace trace{ "queue::handle::dequeue::forget::Request"};

                     local::ipc::eventually::send( message.process, m_state.pending.forget( message));
                  }

               } // forget

            } // dequeue

            namespace peek
            {
               namespace information
               {
                  void Request::operator () ( common::message::queue::peek::information::Request& message)
                  {
                     Trace trace{ "queue::handle::peek::information::Request"};

                     local::ipc::eventually::send( message.process, m_state.queuebase.peek( message));

                  }

               } // information

               namespace messages
               {
                  void Request::operator () ( common::message::queue::peek::messages::Request& message)
                  {
                     Trace trace{ "queue::handle::peek::information::Request"};

                     local::ipc::eventually::send( message.process, m_state.queuebase.peek( message));
                  }
               } // messages

            } // peek

            namespace transaction
            {
               namespace commit
               {
                  void Request::operator () ( common::message::transaction::resource::commit::Request& message)
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
                     }
                     catch( ...)
                     {
                        common::exception::handle();
                        reply.state = common::code::xa::resource_fail;
                     }

                     local::persistent::reply( m_state, message.process, std::move( reply));

                     // handle::persist will take care of pending dequeue requests
                  }
               }

               namespace prepare
               {

                  void Request::operator () ( common::message::transaction::resource::prepare::Request& message)
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
                  void Request::operator () ( common::message::transaction::resource::rollback::Request& message)
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
                     
                     // note: if we want to send directly we need to take care of pending dequeue explicitly
                     local::persistent::reply( m_state, message.process, std::move( reply));

                     // handle::persist will take care of pending dequeue requests
                  }
               }
            } // transaction

            namespace restore
            {
               void Request::operator () ( common::message::queue::restore::Request& message)
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

                  // Make sure we persist and let pending dequeues get a crack at the restored messages.
                  m_state.persist();
                  local::pending::dequeues( m_state);
               }
            } // restore

            namespace signal
            {
               void Timeout::operator () ( const common::message::signal::Timeout&)
               {
                  Trace trace{ "queue::handle::signal::Timeout"};

                  // if there are pending replies waiting for a persistent write,
                  // we don't do anything and let the handle::persist take care
                  // of the pending dequeues, which will happen soon.
                  // otherwise we need to explicit check the newly available messages
                  // (known from the timeout) if there are matches for dequeues...
                  if( m_state.pending.replies.empty())
                     local::pending::dequeues( m_state);
               }
             
            } // signal

         } // handle

         handle::dispatch_type handler( State& state)
         {
            return {
               common::event::listener( handle::dead::Process{ state}),
               handle::enqueue::Request{ state},
               handle::dequeue::Request{ state},
               handle::dequeue::forget::Request{ state},
               handle::signal::Timeout{ state},
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

