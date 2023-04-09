//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/group/handle.h"
#include "queue/common/log.h"
#include "queue/common/ipc/message.h"
#include "queue/common/ipc.h"

#include "common/message/dispatch/handle.h"
#include "common/message/transaction.h"
#include "common/message/signal.h"
#include "common/message/event.h"
#include "common/message/internal.h"

#include "common/event/listen.h"
#include "common/exception/capture.h"
#include "common/execute.h"
#include "common/code/casual.h"
#include "common/code/category.h"
#include "common/signal/timer.h"
#include "common/event/send.h"

#include "common/environment.h"


namespace casual
{
   using namespace common;

   namespace queue::group
   {
      namespace handle
      {
         namespace local
         {
            namespace
            {
               namespace detail
               {
                  namespace transaction
                  {
                     //! blocking "involve" send to TM
                     template< typename M>
                     void involved( State& state, M& message)
                     {
                         Trace trace{ "queue::group::handle::local::detail::transaction::involved"};

                        if( ! algorithm::find( state.involved, message.trid))
                        {
                           // we need to send this blocking to guarantee that TM knows about our
                           // association with the transaction BEFORE any potential prepare/commit 
                           // stage from caller.
                           communication::device::blocking::send( 
                              queue::ipc::transaction::manager(),
                              common::message::transaction::resource::external::involved::create( message));

                           state.involved.push_back( message.trid);
                        }
                        log::line( verbose::log, "state.involve: ", state.involved);
                     }

                     template< typename M>
                     void done( State& state, M& message)
                     {
                        Trace trace{ "queue::group::handle::local::detail::transaction::done"};
                        algorithm::container::erase( state.involved, message.trid);
                        log::line( verbose::log, "state.involved: ", state.involved);
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
                        Trace trace{ "queue::handle::local::dead::process"};

                        // we clear up our own pending state, TM will send us rollback if the
                        // process owned any transactions we have as pending enqueue/dequeue (this 
                        // could have taken place already)
                        state.pending.remove( message.state.pid);
                     };
                  }

               } // dead

               namespace state
               {
                  auto request( State& state)
                  {
                     return [ &state]( queue::ipc::message::group::state::Request& message)
                     {
                        Trace trace{ "queue::handle::local::state::request"};
                        log::line( verbose::log, "message: ", message);

                        auto reply = common::message::reverse::type( message, common::process::handle());
                        reply.queues = state.queuebase.queues();
                        reply.alias = state.alias;
                        reply.queuebase = state.queuebase.file();
                        reply.zombies = state.zombies;

                        state.multiplex.send( message.process.ipc, reply);
                     };
                  }
               } // state

               namespace message
               {
                  namespace meta
                  {
                     auto request( State& state)
                     {
                        return [&state]( queue::ipc::message::group::message::meta::Request& message)
                        {
                           Trace trace{ "queue::handle::local::message::meta::request"};
                           log::line( verbose::log, "message: ", message);

                           auto reply = common::message::reverse::type( message);
                           reply.messages = state.queuebase.meta( message.qid);

                           state.multiplex.send( message.process.ipc, reply);
                        };
                     }
                  } // meta

                  namespace remove
                  {
                     auto request( State& state)
                     {
                        return [ &state]( const queue::ipc::message::group::message::remove::Request& message)
                        {
                           Trace trace{ "queue::handle::local::message::remove::request"};
                           log::line( verbose::log, "message: ", message);

                           auto reply = common::message::reverse::type( message);
                           reply.ids = state.queuebase.remove( message.queue, std::move( message.ids));

                           state.multiplex.send( message.process.ipc, reply);
                        }; 
                     }
                     
                  } // remove

                  namespace recovery
                  {
                     auto request( State& state)
                     {
                        return [&state]( queue::ipc::message::group::message::recovery::Request& message)
                        {
                           Trace trace{ "queue::handle::local::message::recovery::request"};
                           log::line( verbose::log, "message: ", message);

                           auto reply = common::message::reverse::type( message);

                           if( message.directive == decltype( message.directive)::commit)
                              reply.gtrids = state.queuebase.recovery_commit( message.queue, std::move( message.gtrids));
                           else
                              reply.gtrids = state.queuebase.recovery_rollback( message.queue, std::move( message.gtrids));

                           state.multiplex.send( message.process, reply);
                        };
                     }
                  } // recovery
               } // message

               namespace enqueue
               {
                  auto request( State& state)
                  {
                     return [ &state]( queue::ipc::message::group::enqueue::Request& message)
                     {
                        Trace trace{ "queue::handle::enqueue::Request"};
                        log::line( verbose::log, "message: ", message);

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
                              state.multiplex.send( message.process.ipc, reply);
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
                           log::line( log::category::error, exception::capture(), " failed with enqueue request to queue: ", message.name);
                           state.multiplex.send( message.process.ipc, common::message::reverse::type( message));
                        }
                     };
                  }

               } // enqueue

               namespace dequeue
               {
                  bool handle( State& state, queue::ipc::message::group::dequeue::Request& message)
                  {
                     Trace trace{ "queue::handle::dequeue::Request::handle"};
                     log::line( verbose::log, "message: ", message);

                     auto now = platform::time::clock::type::now();

                     auto reply = state.queuebase.dequeue( message, now);
                     reply.correlation = message.correlation;

                     // make sure we always send reply
                     auto send_reply = execute::scope( [&]()
                     {
                        state.multiplex.send( message.process.ipc, reply);
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
                           if( ! current || wanted < current)
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
                     return [&state]( queue::ipc::message::group::dequeue::Request& message)
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
                           log::line( log::category::error, exception::capture(), " failed with dequeue request from queue: ", message.name);
                           return true;
                        }
                        
                     };
                  }

                  namespace forget
                  {
                     auto request( State& state)
                     {
                        return [&state]( queue::ipc::message::group::dequeue::forget::Request& message)
                        {
                           Trace trace{ "queue::handle::local::dequeue::forget::Request"};

                           state.multiplex.send( message.process.ipc, state.pending.forget( message));
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
                        return [ &state]( const queue::ipc::message::group::message::meta::peek::Request& message)
                        {
                           Trace trace{ "queue::handle::local::peek::information::Request"};

                           state.multiplex.send( message.process.ipc, state.queuebase.peek( message));
                        };
                     }

                  } // information

                  namespace messages
                  {
                     auto request( State& state)
                     {
                        return [ &state]( const queue::ipc::message::group::message::peek::Request& message)
                        {
                           Trace trace{ "queue::handle::local::peek::messages::Request"};

                           state.multiplex.send( message.process.ipc, state.queuebase.peek( message));
                        };
                     }
                  } // messages

               } // peek

               namespace browse
               {
                  auto request( State& state)
                  {
                     return [ &state]( const queue::ipc::message::group::message::browse::Request& message)
                     {
                        Trace trace{ "queue::handle::local::browse::Request"};

                        state.multiplex.send( message.process.ipc, state.queuebase.browse( message, platform::time::clock::type::now()));
                     };
                  }
               } // browse

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
                              log::line( log::category::error, exception::capture(), " transaction commit request failed");
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
                        return [ &state]( common::message::transaction::resource::prepare::Request& message)
                        {
                           Trace trace{ "queue::handle::local::transaction::prepare::Request"};
                           log::line( verbose::log, "message: ", message);

                           auto reply = common::message::reverse::type( message, common::process::handle());
                           reply.resource = message.resource;
                           reply.trid = message.trid;
                           reply.state = common::code::xa::ok;

                           state.multiplex.send( message.process.ipc, reply);
                        };
                     }
                  } // prepare

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
                              common::log::line( common::log::category::error, common::code::xa::resource_fail, " transaction rollback request - ", common::exception::capture());
                              reply.state = common::code::xa::resource_fail;
                           }
                           
                           // note: if we want to send directly we need to take care of pending dequeue explicitly
                           local::detail::persistent::reply( state, message.process, std::move( reply));

                           // handle::persist will take care of pending dequeue requests
                        };
                     }
                  } // rollback
               } // transaction

               namespace restore
               {
                  auto request( State& state)
                  {
                     return [&state]( const queue::ipc::message::group::queue::restore::Request& message)
                     {
                        Trace trace{ "queue::handle::local::restore::Request"};
                        log::line( verbose::log, "message: ", message);

                        auto reply = common::message::reverse::type( message);

                        auto send_reply = common::execute::scope( [&]()
                        {
                           state.multiplex.send( message.process.ipc, reply);
                        });

                        reply.affected = common::algorithm::transform( message.queues, [&]( auto id){
                           std::decay_t< decltype( reply.affected.front())> result;
                           result.count = state.queuebase.restore( id);
                           result.queue.id = id;
                           if( auto queue = state.queuebase.queue( id))
                              result.queue.name = queue.value().name;

                           return result;
                        });

                        // Make sure we persist and let pending dequeues get a crack at the restored messages.
                        state.queuebase.persist();
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
                     return [ &state]( const queue::ipc::message::group::queue::clear::Request& message)
                     {
                        Trace trace{ "queue::handle::local::local::clear::request"};
                        log::line( verbose::log, "message: ", message);

                        auto clear_queue = [&state]( auto id)
                        {
                           queue::ipc::message::group::queue::Affected result;
                           result.queue.id = id;
                           result.count = state.queuebase.clear( id);
                           if( auto queue = state.queuebase.queue( id))
                              result.queue.name = queue.value().name;
                           return result;
                        };

                        auto reply = common::message::reverse::type( message);

                        reply.affected = algorithm::transform( message.queues, clear_queue);

                        state.multiplex.send( message.process.ipc, reply);
                     };
                  }
               } // clear

               namespace metric::reset
               { 
                  auto request( State& state)
                  {
                     return [ &state]( const queue::ipc::message::group::metric::reset::Request& message)
                     {
                        Trace trace{ "queue::handle::local::local::metric::reset::request"};
                        log::line( verbose::log, "message: ", message);

                        state.queuebase.metric_reset( message.queues);

                        state.multiplex.send( message.process.ipc, common::message::reverse::type( message));
                     };
                  }
               } // metric::reset

               namespace configuration::update
               {
                  namespace detail
                  {
                     auto update( State& state, std::vector< queuebase::Queue> wanted, 
                        const std::vector<casual::common::strong::queue::id>& remove = {}, 
                        const std::vector<casual::common::strong::queue::id>& zombies = {})
                     {
                        Trace trace{ "queue::handle::local::local::configuration::update::request::detail::update"};
                        log::line( verbose::log, "wanted: ", wanted);                     
                        log::line( verbose::log, "remove: ", remove);
                        log::line( verbose::log, "zombies: ", zombies);


                        state.queuebase.update( wanted, remove);

                        // ok, wanted queues are added, if any. Take care of reply.
                        // TODO remove queues, if any, if possible.
                        auto queues = state.queuebase.queues();

                        return algorithm::transform_if( queues, 
                        []( auto& queue){
                           return queue::ipc::message::group::configuration::update::Queue{ queue.id, queue.name};
                        },
                        [&zombies]( auto& queue){
                           return ! algorithm::find( zombies, queue.id);
                        }
                        );
                     }

                     auto queuebase( const queue::ipc::message::group::configuration::update::Request& message)
                     {
                        if( ! message.model.queuebase.empty())
                           return group::Queuebase{ message.model.queuebase};

                        if( ! message.model.directory.empty())
                           return group::Queuebase{ message.model.directory + "/" + message.model.alias + ".qb"};
                        
                        auto name = message.model.alias + ".qb";
                        auto file = common::environment::directory::queue() / name;

                        // TODO: remove this in 2.0 (that exist to be backward compatible)
                        {
                           // if the wanted path exists, we can't overwrite with the old
                           if( std::filesystem::exists( file))
                              return group::Queuebase{ std::move( file)};

                           auto old = common::environment::directory::domain() / "queue" / "groups" / name;

                           if( std::filesystem::exists( old))
                           {
                              std::filesystem::rename( old, file);
                              common::event::notification::send( "queuebase file moved: ", std::filesystem::relative( old), " -> ", std::filesystem::relative( file));
                              log::line( log::category::warning, "queuebase file moved: ", old, " -> ", file);
                           }
                        }

                        return group::Queuebase{ std::move( file)};
                     }

                  } // detail

                  auto request( State& state)
                  {
                     return [&state]( const queue::ipc::message::group::configuration::update::Request& message)
                     {
                        Trace trace{ "queue::handle::local::configuration::update::request"};
                        log::line( verbose::log, "message: ", message);

                        // this can't be updated if once set (yet)
                        if( ! state.queuebase)
                           state.queuebase = detail::queuebase( message);

                        // we persist and start a 'local transaction'
                        state.queuebase.persist();
                        auto rollback = execute::scope( [&state](){ state.queuebase.rollback();});

                        state.alias = message.model.alias;
                        state.note = message.model.note;

                        auto wanted = algorithm::transform( message.model.queues, []( auto& queue)
                        {
                           return queuebase::Queue{ queue.name, { queue.retry.count, queue.retry.delay}};
                        });

                        log::line( verbose::log, "wanted: ", wanted);

                        auto existing = state.queuebase.queues();
                        // correlate existing ids
                        {
                           log::line( verbose::log, "existing: ", existing);

                           auto correlate_id = [&existing]( auto& queue)
                           {
                              if( auto found = algorithm::find( existing, queue.name))
                              {
                                 queue.id = found->id;
                                 queue.error = found->error;
                              }
                           };

                           algorithm::for_each( wanted, correlate_id);
                        }

                        auto zombies = std::get< 1>( algorithm::intersection( existing, wanted, []( auto& exists, auto& wants)
                        {
                           return exists.id == wants.id || exists.id == wants.error;
                        }));

                        using zombie_type = decltype( zombies)::value_type;
                        std::vector< std::tuple< zombie_type, zombie_type> > joined;

                        auto join_with_error_queue = [&joined, &zombies]( auto& queue)
                        {
                           if( auto found = algorithm::find_if( zombies, 
                              [&queue](auto& item){ return queue.error == item.id;}))
                           {
                              joined.emplace_back( std::make_tuple( queue, *found));
                           }
                        };

                        using Type = queue::ipc::message::group::state::queue::Type;
                        algorithm::for_each_if( zombies, join_with_error_queue, []( auto& queue){ return queue.type() == Type::queue;});

                        auto has_messages = []( auto& item){ return std::get< 0>( item).metric.count > 0 || std::get< 1>( item).metric.count > 0;};

                        auto [ nonempty_queues, empty_queues] = algorithm::partition( joined, has_messages);

                        log::line( verbose::log, "nonempty_queues: ", nonempty_queues);
                        log::line( verbose::log, "empty_queues: ", empty_queues);

                        std::vector<casual::common::strong::queue::id> remove;

                        auto populate = []( auto& queues, auto& out){
                           algorithm::transform( queues, 
                              std::back_inserter( out), 
                              []( auto& item) { return std::get< 0>( item).id;});
                           algorithm::transform( queues, 
                              std::back_inserter( out), 
                              []( auto& item) { return std::get< 1>( item).id;});
                        };

                        populate( empty_queues, remove);
                        populate( nonempty_queues, state.zombies);

                        log::line( verbose::log, "remove: ", remove);

                        // if something goes wrong we send fatal event
                        auto queues = common::event::guard::fatal( [&]()
                        { 
                           return detail::update( state, wanted, remove, state.zombies);
                        });

                        {
                           auto reply = common::message::reverse::type( message, process::handle());
                           reply.alias = state.alias;
                           reply.queues = std::move( queues);
                           state.multiplex.send( message.process.ipc, reply);
                        }

                        algorithm::for_each( nonempty_queues, []( auto& queuepair)
                        {
                           common::log::line( common::log::category::warning, "reconfigure zombie queues and remove messages to remove it: ", std::get< 0>( queuepair).name, " - ", std::get<1>( queuepair).name);
                        });
 
                        log::line( verbose::log, "state: ", state);

                        // everything went ok, do not rollback.
                        rollback.release();
                        state.queuebase.persist();
                     };
                  }
               } // configuration::update

               namespace shutdown
               { 
                  namespace detail
                  {
                     void perform( State& state)
                     {
                        Trace trace{ "queue::handle::shutdown"};

                        state.runlevel = decltype( state.runlevel())::shutdown;

                        handle::persist( state);

                        // send _forget requests_ to pending dequeue requests, if any.
                        state.pending.forget().send( state.multiplex);
                     }
                  } // detail

                  auto request( State& state)
                  {
                     return [&state]( const common::message::shutdown::Request& message)
                     {
                        Trace trace{ "queue::handle::local::shutdown::request"};
                        log::line( verbose::log, "message: ", message);

                        detail::perform( state);

                        log::line( verbose::log, "state: ", state);
                     };
                  }
               } // shutdown

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

                        // take care of not available ( yet). We find the earliest and set a timer for that
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

         void abort( State& state)
         {
            Trace trace{ "queue::handle::abort"};

            state.runlevel = decltype( state.runlevel())::error;
            local::shutdown::detail::perform( state);
         }

         void persist( State& state)
         {
            Trace trace{ "queue::handle::persist"};

            if( state.pending.replies.empty())
               return; // nothing to do.

            // persist the queuebase
            state.queuebase.persist();
            
            state.pending.replies.send( state.multiplex);

            // handle pending dequeues, if any.
            local::detail::pending::dequeues( state);
         }

      } // handle

      handle::dispatch_type handlers( State& state)
      {
         return handle::dispatch_type{
            common::event::listener( handle::local::dead::process( state)),
            common::message::dispatch::handle::defaults( state),

            handle::local::configuration::update::request( state),
            handle::local::enqueue::request( state),
            handle::local::dequeue::request( state),
            handle::local::dequeue::forget::request( state),
            handle::local::signal::timeout( state),
            handle::local::transaction::commit::request( state),
            handle::local::transaction::prepare::request( state),
            handle::local::transaction::rollback::request( state),
            handle::local::state::request( state),
            handle::local::peek::information::request( state),
            handle::local::peek::messages::request( state),
            handle::local::browse::request( state),
            handle::local::restore::request( state),
            handle::local::clear::request( state),
            handle::local::message::meta::request( state),
            handle::local::message::remove::request( state),
            handle::local::message::recovery::request( state),
            handle::local::metric::reset::request( state),
            handle::local::shutdown::request( state),
         };
      }

   } // queue::group
} // casual

