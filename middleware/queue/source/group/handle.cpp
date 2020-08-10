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
#include "common/code/casual.h"
#include "common/code/category.h"

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
               communication::ipc::inbound::Device& device()
               {
                  return communication::ipc::inbound::device();
               }
            } // ipc


            namespace local
            {
               namespace
               {
                  namespace detail
                  {
                     namespace ipc
                     {
                        namespace eventually
                        {
                           template< typename M>
                           void send( const process::Handle& destination, M&& message)
                           {
                              try
                              {  
                                 if( ! communication::device::non::blocking::send(  destination.ipc, message))
                                    casual::domain::pending::message::send( destination, message);
                              }
                              catch( ...)
                              {
                                 auto code = exception::code();
                                 
                                 if( code != code::casual::communication_unavailable)
                                    throw;

                                 log::line( log, code, " destination not available: ", destination, " - action: ignore");
                                 log::line( verbose::log, "dropped message: ", message);
                              }                         
                           }
                        } // eventually

                        namespace pending
                        {
                           void send( common::message::pending::Message& pending)
                           {
                              if( ! common::message::pending::non::blocking::send( pending))
                                 casual::domain::pending::message::send( pending);
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
                              communication::device::blocking::optional::send( 
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

                           if( state.pending.replies.size() >= platform::batch::queue::persistent)
                              handle::persist( state);
                        }

                     } // persistent

                     namespace pending
                     { 
                        // defined further down.
                        void dequeues( State& state);
                        
                     } // pending
                  } // detail

                  namespace dead
                  {
                     auto process( State& state)
                     {
                        return [&state]( const common::message::event::process::Exit& message)
                        {
                           Trace trace{ "queue::handle::dead::Process"};

                           // we clear up our own pending state, TM will send us rollback if the
                           // process owned any transactions we have as pending enqueue/dequeue (this 
                           // could have taken place already)
                           state.pending.remove( message.state.pid);
                        };
                     }

                  } // dead

                  namespace information
                  {
                     namespace queues
                     {
                        auto request( State& state)
                        {
                           return [&state]( common::message::queue::information::queues::Request& message)
                           {
                              Trace trace{ "queue::handle::information::queues::request"};
                              log::line( verbose::log, "message: ", message);

                              auto reply = common::message::reverse::type( message, common::process::handle());
                              reply.queues = state.queuebase.queues();

                              common::communication::device::blocking::send( message.process.ipc, reply);
                           };
                        }
                     } // queues

                     namespace messages
                     {
                        auto request( State& state)
                        {
                           return [&state]( common::message::queue::information::messages::Request& message)
                           {
                              Trace trace{ "queue::handle::information::messages::request"};
                              log::line( verbose::log, "message: ", message);

                              auto reply = common::message::reverse::type( message, common::process::handle());
                              reply.messages = state.queuebase.messages( message.qid);

                              common::communication::device::blocking::send( message.process.ipc, reply);
                           };
                        }
                     } // messages

                  } // information


                  namespace enqueue
                  {
                     auto request( State& state)
                     {
                        return [&state]( common::message::queue::enqueue::Request& message)
                        {
                           Trace trace{ "queue::handle::enqueue::Request"};

                           try 
                           {
                              // Make sure we've got the quid.
                              message.queue = state.queuebase.id( message);

                              auto reply = state.queuebase.enqueue( message);

                              if( message.trid)
                              {
                                 // for clarification: TM is guaranteed to consume 
                                 // the involved message before caller issue any 
                                 // transaction messages of their own (since
                                 // we send 'involved' first).
                                 local::detail::transaction::involved( state, message);
                                 communication::device::blocking::optional::send( message.process.ipc, reply);
                              }
                              else
                              {
                                 // enqueue is not in transaction, we guarantee atomic enqueue so
                                 // we send reply when we're in persistent state
                                 local::detail::persistent::reply( state, message.process, std::move( reply));

                                 // handle::persist will take care of pending dequeue requests
                              }
                           }
                           catch( ...)
                           {
                              exception::handle( log::category::error, "failed with enqueue request");
                           }
                        };
                     }

                  } // enqueue

                  namespace dequeue
                  {
                     bool handle( State& state, common::message::queue::dequeue::Request& message)
                     {
                        Trace trace{ "queue::handle::dequeue::Request::handle"};
                        log::line( verbose::log, "message: ", message);

                        auto now = platform::time::clock::type::now();

                        auto reply = state.queuebase.dequeue( message, now);
                        reply.correlation = message.correlation;

                        // make sure we always send reply
                        auto send_reply = execute::scope( [&]()
                        {
                           communication::device::blocking::optional::send( message.process.ipc, reply);
                        });

                        if( ! reply.message.empty())
                        {
                           // we notify TM if the dequeue is in a transaction
                           if( message.trid)
                              local::detail::transaction::involved( state, message);
                        }
                        else if( message.block)
                        {
                           // check if we need to set a timer
                           auto available = state.queuebase.available( message.queue);
                           if( available)
                           {
                              auto wanted = available.value() - now;
                              auto current = common::signal::timer::get();
                              log::line( verbose::log, "wanted: ", wanted, ", current: ", current);
                              if( current == platform::time::unit::min() || wanted < current)
                                 common::signal::timer::set( wanted);
                           }

                           // no message, but caller wants to block
                           state.pending.add( std::move( message));

                           // we don't send reply
                           send_reply.release();

                           return false;
                        }
                        return true;
                     }

                     auto request( State& state)
                     {
                        return [&state]( common::message::queue::dequeue::Request& message)
                        {
                           Trace trace{ "queue::handle::local::dequeue::Request"};

                           try
                           {
                              // Make sure we've got the quid.
                              message.queue = state.queuebase.id( message);

                              return handle( state, message);
                           }
                           catch( ...)
                           {
                              exception::handle( log::category::error, "failed with dequeue request");
                              return true;
                           }
                           
                        };
                     }

                     namespace forget
                     {
                        auto request( State& state)
                        {
                           return [&state]( common::message::queue::dequeue::forget::Request& message)
                           {
                              Trace trace{ "queue::handle::local::dequeue::forget::Request"};

                              detail::ipc::eventually::send( message.process, state.pending.forget( message));
                           };
                        }
                     } // forget

                  } // dequeue

                  namespace peek
                  {
                     namespace information
                     {
                        auto request( State& state)
                        {
                           return [&state]( const common::message::queue::peek::information::Request& message)
                           {
                              Trace trace{ "queue::handle::local::peek::information::Request"};

                              detail::ipc::eventually::send( message.process, state.queuebase.peek( message));
                           };
                        }

                     } // information

                     namespace messages
                     {
                        auto request( State& state)
                        {
                           return [&state]( const common::message::queue::peek::messages::Request& message)
                           {
                              Trace trace{ "queue::handle::local::peek::information::Request"};

                              detail::ipc::eventually::send( message.process, state.queuebase.peek( message));
                           };
                        }
                     } // messages

                  } // peek

                  namespace transaction
                  {
                     namespace commit
                     {
                        auto request( State& state)
                        {
                           return [&state]( common::message::transaction::resource::commit::Request& message)
                           {
                              Trace trace{ "queue::handle::local::transaction::commit::Request"};
                              log::line( verbose::log, "message: ", message);

                              local::detail::transaction::done( state, message);

                              auto reply = common::message::reverse::type( message, common::process::handle());
                              reply.resource = message.resource;
                              reply.trid = message.trid;
                              reply.state = common::code::xa::ok;

                              try
                              {
                                 state.queuebase.commit( message.trid);
                                 log::line( log::category::transaction, "committed trid: ", message.trid, " - number of messages: ", state.queuebase.affected());
                              }
                              catch( ...)
                              {
                                 common::exception::handle( common::log::category::error, "transaction commit request");
                                 reply.state = common::code::xa::resource_fail;
                              }

                              local::detail::persistent::reply( state, message.process, std::move( reply));

                              // handle::persist will take care of pending dequeue requests
                           };
                        }
                     }

                     namespace prepare
                     {
                        auto request( State& state)
                        {
                           return []( common::message::transaction::resource::prepare::Request& message)
                           {
                              Trace trace{ "queue::handle::local::transaction::prepare::Request"};
                              log::line( verbose::log, "message: ", message);

                              auto reply = common::message::reverse::type( message, common::process::handle());
                              reply.resource = message.resource;
                              reply.trid = message.trid;
                              reply.state = common::code::xa::ok;

                              communication::device::blocking::optional::send(
                                 common::communication::instance::outbound::transaction::manager::device(), reply);
                           };
                        }
                     }

                     namespace rollback
                     {
                        auto request( State& state)
                        {
                           return [&state]( common::message::transaction::resource::rollback::Request& message)
                           {
                              Trace trace{ "queue::handle::local::transaction::rollback::Request"};
                              log::line( verbose::log, "message: ", message);

                              local::detail::transaction::done( state, message);

                              auto reply = common::message::reverse::type( message, common::process::handle());
                              reply.resource = message.resource;
                              reply.trid = message.trid;
                              reply.state = common::code::xa::ok;

                              try
                              {
                                 state.queuebase.rollback( message.trid);
                                 common::log::line( common::log::category::transaction, "rollback trid: ", message.trid, 
                                    " - number of messages: ", state.queuebase.affected());
                              }
                              catch( ...)
                              {
                                 common::log::line( common::log::category::error, common::code::xa::resource_fail, " transaction rollback request - ", common::exception::code());
                                 reply.state = common::code::xa::resource_fail;
                              }
                              
                              // note: if we want to send directly we need to take care of pending dequeue explicitly
                              local::detail::persistent::reply( state, message.process, std::move( reply));

                              // handle::persist will take care of pending dequeue requests
                           };
                        }
                     }
                  } // transaction

                  namespace restore
                  {
                     auto request( State& state)
                     {
                        return [&state]( const common::message::queue::restore::Request& message)
                        {
                           Trace trace{ "queue::handle::local::restore::Request"};
                           log::line( verbose::log, "message: ", message);

                           auto reply = common::message::reverse::type( message);

                           auto send_reply = common::execute::scope( [&](){
                              communication::device::blocking::optional::send( message.process.ipc, reply);
                           });

                           reply.affected = common::algorithm::transform( message.queues, [&]( auto queue){
                              std::decay_t< decltype( reply.affected.front())> result;
                              result.queue = queue;
                              result.count = state.queuebase.restore( queue);
                              return result;
                           });

                           // Make sure we persist and let pending dequeues get a crack at the restored messages.
                           state.persist();
                           local::detail::pending::dequeues( state);
                        };
                     }
                  } // restore

                  namespace signal
                  {
                     auto timeout( State& state)
                     {
                        return [&state]( const common::message::signal::Timeout&)
                        {
                           Trace trace{ "queue::handle::local::signal::Timeout"};

                           // if there are pending replies waiting for a persistent write,
                           // we don't do anything and let the handle::persist take care
                           // of the pending dequeues, which will happen soon.
                           // otherwise we need to explicit check the newly available messages
                           // (known from the timeout) if there are matches for dequeues...
                           if( state.pending.replies.empty())
                              local::detail::pending::dequeues( state);
                        };
                     }
                  } // signal

                  namespace clear
                  {
                     auto request( State& state)
                     {
                        return [&state]( const common::message::queue::clear::Request& message)
                        {
                           Trace trace{ "queue::handle::local::local::clear::request"};
                           log::line( verbose::log, "message: ", message);

                           auto clear_queue = [&state]( auto id)
                           {
                              common::message::queue::Affected result;
                              result.queue = id;
                              result.count = state.queuebase.clear( id);
                              return result;
                           };

                           auto reply = common::message::reverse::type( message);

                           reply.affected = algorithm::transform( message.queues, clear_queue);

                           detail::ipc::eventually::send( message.process, reply);
                        };
                     }
                  } // clear

                  namespace messages
                  {
                     namespace remove
                     {
                        auto request( State& state)
                        {
                           return [&state]( const common::message::queue::messages::remove::Request& message)
                           {
                              Trace trace{ "queue::handle::local::local::messages::remove::request"};
                              log::line( verbose::log, "message: ", message);

                              auto reply = common::message::reverse::type( message);

                              reply.ids = state.queuebase.remove( message.queue, std::move( message.ids));

                              detail::ipc::eventually::send( message.process, reply);
                           }; 
                        }
                        
                     } // remove
                  } // messages

                  namespace metric
                  {
                     namespace reset
                     {
                        auto request( State& state)
                        {
                           return [&state]( const common::message::queue::metric::reset::Request& message)
                           {
                              Trace trace{ "queue::handle::local::local::metric::reset::request"};
                              log::line( verbose::log, "message: ", message);

                              state.queuebase.metric_reset( message.queues);

                              detail::ipc::eventually::send( message.process, common::message::reverse::type( message, common::process::handle()));
                           };

                        }
                     } // reset
                  } // metric

                  namespace detail
                  {
                     namespace pending
                     { 
                        // Take care of pending/blocking dequeues.
                        // defined here ("last") since it uses dequeue::request above
                        void dequeues( State& state)
                        {
                           Trace trace{ "queue::handle::local::detail::pending::dequeues"};

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

                              auto dequeue =  handle::local::dequeue::request( state);

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
                  } // detail
               } // <unnamed>
            } // local


            void shutdown( State& state)
            {
               Trace trace{ "queue::handle::shutdown"};

               handle::persist( state);

               // send _forget requests_ to pending dequeue requests, if any.
               algorithm::for_each( state.pending.forget(), &local::detail::ipc::pending::send);

            }

            void persist( State& state)
            {
               Trace trace{ "queue::handle::persist"};

               if( state.pending.replies.empty())
                  return; // nothing to do.

               // persist the queuebase
               state.persist();
               
               algorithm::for_each( std::exchange( state.pending.replies, {}), &local::detail::ipc::pending::send);

               // handle pending dequeues, if any.
               local::detail::pending::dequeues( state);
            }

         } // handle

         handle::dispatch_type handler( State& state)
         {
            return {
               common::event::listener( handle::local::dead::process( state)),
               handle::local::enqueue::request( state),
               handle::local::dequeue::request( state),
               handle::local::dequeue::forget::request( state),
               handle::local::signal::timeout( state),
               handle::local::transaction::commit::request( state),
               handle::local::transaction::prepare::request( state),
               handle::local::transaction::rollback::request( state),
               handle::local::information::queues::request( state),
               handle::local::information::messages::request( state),
               handle::local::peek::information::request( state),
               handle::local::peek::messages::request( state),
               handle::local::restore::request( state),
               handle::local::clear::request( state),
               handle::local::messages::remove::request( state),
               handle::local::metric::reset::request( state),
               common::message::handle::Shutdown{},
            };
         }

      } // server
   } // queue
} // casual

