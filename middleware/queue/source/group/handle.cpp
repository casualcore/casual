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

#include "domain/discovery/api.h"

#include "configuration/model/change.h"

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
                        log::debug( "state.involve: ", state.involved);
                     }

                     template< typename M>
                     void done( State& state, M& message)
                     {
                        Trace trace{ "queue::group::handle::local::detail::transaction::done"};
                        algorithm::container::erase( state.involved, message.trid);
                        log::debug( "state.involved: ", state.involved);
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
                     return [ &state]( const common::message::event::process::Exit& message)
                     {
                        Trace trace{ "queue::handle::local::dead::process"};
                        log::debug( "message", message);

                        // we clear up our own pending state, TM will send us rollback if the
                        // process owned any transactions we have as pending enqueue/dequeue (this 
                        // could have taken place already)
                        state.pending.remove( message.state.pid);
                     };
                  }

                  auto ipc( State& state)
                  {
                     return [ &state]( const common::message::event::ipc::Destroyed& message)
                     {
                        Trace trace{ "queue::handle::local::dead::ipc"};
                        log::debug( "message", message);

                        // we clear up our own pending state, TM will send us rollback if the
                        // process owned any transactions we have as pending enqueue/dequeue (this 
                        // could have taken place already)
                        state.pending.remove( message.process.ipc);
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
                        log::debug( "message: ", message);

                        auto reply = common::message::reverse::type( message, common::process::handle());
                        reply.queues = state.queuebase.queues();
                        reply.alias = state.alias;
                        reply.queuebase = state.queuebase.file();
                        reply.zombies = state.zombies;
                        reply.size.current = state.size.current;
                        reply.size.capacity = state.size.capacity;

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
                           log::debug( "message: ", message);

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
                           log::debug( "message: ", message);

                           auto reply = common::message::reverse::type( message);
                           reply.ids = message.force ?
                              state.queuebase.force_remove( message.queue, std::move( message.ids)) :
                              state.queuebase.remove( message.queue, std::move( message.ids));

                           state.size.current = state.queuebase.size();

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
                           log::debug( "message: ", message);

                           auto reply = common::message::reverse::type( message);

                           auto [ size, gtrids] = message.directive == decltype( message.directive)::commit ?
                              state.queuebase.recovery_commit( message.queue, std::move( message.gtrids)) :
                              state.queuebase.recovery_rollback( message.queue, std::move( message.gtrids));

                           state.size.subtract( size);
                           reply.gtrids = gtrids;

                           state.multiplex.send( message.process, reply);
                        };
                     }
                  } // recovery
               } // message

               namespace enqueue
               {
                  namespace detail
                  {
                     namespace payload::size
                     {
                        platform::size::type calculate( const queue::ipc::message::group::enqueue::Request& message)
                        {
                           return message.message.payload.data.size();
                        }
                     } // payload::size

                     namespace has::sufficient
                     {
                        bool capacity( State& state, const queue::ipc::message::group::enqueue::Request& message)
                        {
                           if( ! state.size.capacity)
                              return true;
                           
                           auto available = state.size.capacity.value() - state.size.current;
                           return available >= payload::size::calculate( message);
                        }
                     } // has::sufficient
                  } // detail

                  auto request( State& state)
                  {
                     return [ &state]( queue::ipc::message::group::enqueue::Request& message)
                     {
                        Trace trace{ "queue::handle::enqueue::Request"};
                        log::debug( "message: ", message);

                        if( ! detail::has::sufficient::capacity( state, message))
                        {
                           log::line( log::category::error, " failed with enqueue request to queue: ", message.name, " - queue-group ", state.alias, " full");
                           auto reply = common::message::reverse::type( message);
                           reply.code = common::code::queue::no_queue;  // bespoke code for 'queuebase_full'?
                           state.multiplex.send( message.process.ipc, reply);
                           return;
                        }

                        try 
                        {
                           // Make sure we've got the quid.
                           message.queue = state.queuebase.id( message);

                           auto reply = state.queuebase.enqueue( message);

                           state.size.add( detail::payload::size::calculate( message));

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
                           auto reply = common::message::reverse::type( message);

                           auto error = exception::capture();
                           if( code::is::category< code::queue>( error.code()))
                              reply.code = static_cast< code::queue>( error.code().value());
                           else
                              reply.code = decltype( reply.code)::system;

                           log::error( reply.code, " failed with enqueue request to queue: ", message.name, " - ", error);
                           state.multiplex.send( message.process.ipc, reply);
                        }
                     };
                  }

               } // enqueue

               namespace dequeue
               {
                  bool handle( State& state, queue::ipc::message::group::dequeue::Request& message)
                  {
                     Trace trace{ "queue::handle::dequeue::Request::handle"};
                     log::debug( "message: ", message);

                     // Make sure we've got the quid.
                     message.queue = state.queuebase.id( message);
                     auto now = platform::time::clock::type::now();

                     auto reply = state.queuebase.dequeue( message, now);
                     reply.correlation = message.correlation;

                     if( reply.code == decltype( reply.code)::ok)
                     {
                        // we notify TM if the dequeue is in a transaction
                        if( message.trid)
                           local::detail::transaction::involved( state, message);
                        else
                           state.size.subtract( reply.message.value().payload.data.size());

                        state.multiplex.send( message.process.ipc, reply);
                     }
                     else if( message.block)
                     {
                        // check if we need to set a timer
                        auto available = state.queuebase.available( message.queue);
                        if( available)
                        {
                           auto wanted = available.value() - now;
                           auto current = common::signal::timer::get();
                           log::debug( "wanted: ", wanted, ", current: ", current);
                           if( ! current || wanted < current)
                              common::signal::timer::set( wanted);
                        }

                        // no message, but caller wants to block
                        state.pending.add( std::move( message));
                        return false;
                     }
                     else
                        state.multiplex.send( message.process.ipc, reply);

                     return true;
                  }

                  auto request( State& state)
                  {
                     return [&state]( queue::ipc::message::group::dequeue::Request& message)
                     {
                        Trace trace{ "queue::handle::local::dequeue::Request"};

                        try
                        {
                           return handle( state, message);
                        }
                        catch( ...)
                        {
                           auto reply = common::message::reverse::type( message);
                           auto error = exception::capture();

                           if( code::is::category< code::queue>( error.code()))
                              reply.code = static_cast< code::queue>( error.code().value());
                           else
                              reply.code = decltype( reply.code)::system;

                           log::error( reply.code, " failed with dequeue request to queue: ", message.name, " - ", error);
                           state.multiplex.send( message.process.ipc, reply);

                           return false;
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
                           log::debug( "message: ", message);

                           local::detail::transaction::done( state, message);

                           auto reply = common::message::reverse::type( message, common::process::handle());
                           reply.resource = message.resource;
                           reply.trid = message.trid;
                           reply.state = common::code::xa::ok;

                           try
                           {
                              state.size.subtract( state.queuebase.commit( message.trid));
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
                           log::debug( "message: ", message);

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
                           log::debug( "message: ", message);

                           local::detail::transaction::done( state, message);

                           auto reply = common::message::reverse::type( message, common::process::handle());
                           reply.resource = message.resource;
                           reply.trid = message.trid;
                           reply.state = common::code::xa::ok;

                           try
                           {
                              state.size.subtract( state.queuebase.rollback( message.trid));
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
                        log::debug( "message: ", message);

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
                        log::debug( "message: ", message);

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

                        state.size.current = state.queuebase.size();

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
                        log::debug( "message: ", message);

                        state.queuebase.metric_reset( message.queues);

                        state.multiplex.send( message.process.ipc, common::message::reverse::type( message));
                     };
                  }
               } // metric::reset

               namespace configuration::update
               {
                  namespace detail
                  {
                     auto queuebase( const queue::ipc::message::group::configuration::update::Request& message)
                     {
                        if( ! message.model.queuebase.empty())
                           return group::Queuebase{ message.model.queuebase};

                        // sanity check
                        if( message.model.alias.empty())
                           common::event::error::raise( common::code::casual::internal_unexpected_value, "queuebase alias is empty");

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

                     // normalizing structure for easier handling
                     struct Queue
                     {
                        common::strong::queue::id id;
                        std::string name;
                        common::strong::queue::id error;
                        queuebase::queue::Retry retry;
                        bool empty = true;

                        inline queuebase::queue::Type type() const { return error ? queuebase::queue::Type::queue : queuebase::queue::Type::error_queue;}

                        inline friend bool operator == ( const Queue& lhs, std::string_view rhs) { return lhs.name == rhs;}
                        inline friend bool operator == ( const Queue& lhs, const Queue& rhs) 
                        { 
                           return lhs.name == rhs.name && lhs.retry == rhs.retry; 
                        }


                        CASUAL_CONST_CORRECT_SERIALIZE(
                           CASUAL_SERIALIZE( id);
                           CASUAL_SERIALIZE( name);
                           CASUAL_SERIALIZE( error);
                           CASUAL_SERIALIZE( retry);
                           CASUAL_SERIALIZE( empty);
                        )

                     };

                     auto transform_normalized()
                     {
                        return casual::overload::compose(
                           []( const casual::configuration::model::queue::Queue& queue)
                           {
                              return detail::Queue{ 
                                 .name = queue.name, 
                                 .retry = { 
                                    .count = queue.retry.count, 
                                    .delay = queue.retry.delay}
                              };
                           },
                           []( const queue::ipc::message::group::state::Queue& queue)
                           {
                              return detail::Queue{ 
                                 .id = queue.id,
                                 .name = queue.name,
                                 .error = queue.error,
                                 .retry = { 
                                    .count = queue.retry.count, 
                                    .delay = queue.retry.delay},
                                 .empty = queue.metric.count == 0
                              
                              };
                           }
                        );
                     }

                     auto transform_queuebase()
                     {
                        return []( const detail::Queue& queue)
                        {
                           return queuebase::Queue{ 
                              .id = queue.id,
                              .name = queue.name,
                              .retry = queuebase::queue::Retry{ 
                                 .count = queue.retry.count, 
                                 .delay = queue.retry.delay},
                              .error = queue.error
                           };
                        };
                     }

                     constexpr auto transform_id = []( const auto& queue)
                     {
                        return queue.id;
                     };

                     constexpr auto transform_error = []( const auto& queue)
                     {
                        return queue.error;
                     };

                     namespace predicate
                     {
                        constexpr auto is_queue = []( const detail::Queue& queue)
                        {
                           return queue.type() == decltype( queue.type())::queue;
                        };

                        constexpr auto is_empty = []( const detail::Queue& queue)
                        {
                           return queue.empty;
                        };


                        auto is_id( const std::vector< common::strong::queue::id>& ids)
                        {
                           return [ &ids]( const auto& queue) -> bool
                           {
                              return algorithm::contains( ids, queue.id);
                           };
                        }

                        constexpr auto equal_name = []( const detail::Queue& lhs, const detail::Queue& rhs)
                        {
                           return lhs.name == rhs.name;
                        };

                        
                     } // predicate

                     // update empty state of queues based on their error queue
                     auto update_empty( auto error_queues)
                     {
                        return [ error_queues]( detail::Queue& queue)
                        {
                           // already known to be non-empty
                           if( ! queue.empty)
                              return;

                           auto is_error = [ error_id = queue.error]( const detail::Queue& queue)
                           {
                              return error_id == queue.id;
                           };

                           if( auto found = algorithm::find_if( error_queues, is_error))
                           {
                              queue.empty = found->empty;
                           }
                        };
                     }

                     auto update_ids( std::span< const detail::Queue> current)
                     {
                        return [ current]( detail::Queue& queue)
                        {
                           if( queue.id)
                              return;

                           if( auto found = algorithm::find( current, queue.name))
                           {
                              queue.id = found->id;
                              queue.error = found->error;
                           }
                        };
                     }

                     auto update( State& state, std::vector< queuebase::Queue> wanted, 
                        const std::vector<casual::common::strong::queue::id>& remove_ids = {}, 
                        const std::vector<casual::common::strong::queue::id>& zombie_ids = {})
                     {
                        Trace trace{ "queue::handle::local::local::configuration::update::request::detail::update"};
                        log::debug( "wanted: ", wanted);                     
                        log::debug( "remove: ", remove_ids);
                        log::debug( "zombies: ", zombie_ids);

                        // if the update created new queues, we need notify discovery since there could be some
                        // other domain that needs to know about the new queues.
                        if( ! state.queuebase.update( std::move( wanted), remove_ids).empty())
                           casual::domain::discovery::discoverable::advertised( state.multiplex);

                        // ok, wanted queues are added, if any. Take care of reply. We need to reply with all 
                        // our non-zombie queues
                        auto existing = state.queuebase.queues();

                        // partition out main queue zombies (zombie_ids are main queues)
                        auto [ zombies, queues] = algorithm::partition( existing, detail::predicate::is_id( zombie_ids));

                        // we need to remove all zombies error queues as well, extract all error ids
                        auto zombie_error_ids = algorithm::transform( zombies, detail::transform_error);

                        // partition out error queues that have zombies as main queues, which leaves us with valid queues
                        auto [ zombie_error_queues, valid_queues] = algorithm::partition( queues, detail::predicate::is_id( zombie_error_ids));

                        // update/replace zombie state
                        {
                           state.zombies = algorithm::transform( zombie_error_queues, detail::transform_id);
                           // g++13 does not have append_range
                           // state.zombies.append_range( zombie_ids);
                           state.zombies.insert( std::end( state.zombies), std::begin( zombie_ids), std::end( zombie_ids));
                           // keep it sorted for easier reading of state-dump, and such
                           std::ranges::sort( state.zombies);
                        }
                        
                        return algorithm::transform( valid_queues, []( auto& queue)
                        {
                           return queue::ipc::message::group::configuration::update::Queue{ queue.id, queue.name};
                        });
                     }

                     struct Update
                     {
                         // new or changed queues
                        std::vector< queuebase::Queue> update;
                        std::vector< common::strong::queue::id> remove;
                        std::vector< common::strong::queue::id> zombies;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                           CASUAL_SERIALIZE( update);
                           CASUAL_SERIALIZE( remove);
                           CASUAL_SERIALIZE( zombies);
                        )
                     };

                     auto calculate_update( std::span< Queue> current, std::span< Queue> wanted)
                     {
                        Trace trace{ "queue::handle::local::local::configuration::update::request::detail::calculate_update"};

                        Update result;

                        // calculate the change we need to perform to conform to wanted state
                        auto change = casual::configuration::model::change::calculate( current, wanted, detail::predicate::equal_name);

                        log::debug( "change: ", change);

                        // take care of removed queues
                        {
                           auto [ to_remove, non_empty] = algorithm::partition( change.removed, detail::predicate::is_empty);

                           result.remove = algorithm::transform( to_remove, detail::transform_id);
                           // non empty queues becomes zombies
                           result.zombies = algorithm::transform( non_empty, detail::transform_id);

                           if( non_empty)
                              common::log::warning( common::code::casual::invalid_configuration, 
                                 "the following queues (or their error queues) are non-empty and will be marked as zombies: ", 
                                 algorithm::transform( non_empty, []( auto& queue){ return queue.name;}));
                        }

                        // take care of updates
                        {
                           result.update = algorithm::transform( change.modified, detail::transform_queuebase());
                        }

                        // take care of added
                        {
                           algorithm::transform( change.added, 
                              std::back_inserter( result.update), 
                              detail::transform_queuebase());
                        }

                        return result;

                     }

                  } // detail

                  auto request( State& state)
                  {
                     return [&state]( const queue::ipc::message::group::configuration::update::Request& message)
                     {
                        Trace trace{ "queue::handle::local::configuration::update::request"};
                        log::debug( "message: ", message);

                        // this can't be updated if once set (yet)
                        if( ! state.queuebase)
                           state.queuebase = detail::queuebase( message);

                        // we persist and start a 'local transaction'
                        state.queuebase.persist();
                        auto rollback = execute::scope( [&state](){ state.queuebase.rollback();});

                        state.alias = message.model.alias;
                        state.note = message.model.note;
                        state.size.capacity = message.model.capacity;

                        // all existing queues, including error queues
                        auto existing = algorithm::transform( state.queuebase.queues(), detail::transform_normalized());

                        // current queues, excluding error queues (we only use error queues to update empty state)
                        auto [ queues, error_queues] = algorithm::partition( existing, detail::predicate::is_queue);

                        // update empty state to take error queues into account
                        algorithm::for_each( queues, detail::update_empty( error_queues));

                        auto wanted = algorithm::transform( message.model.queues, detail::transform_normalized());

                        // we need to update/correlate ids for wanted queues
                        algorithm::for_each( wanted, detail::update_ids( existing));

                        auto update = detail::calculate_update( queues, wanted);

                        log::debug( "update: ", update);

                        // if something goes wrong we send fatal event
                        common::event::guard::fatal( [&]()
                        {
                           auto reply = common::message::reverse::type( message, process::handle());
                           reply.alias = state.alias;

                           // the actual update
                           reply.queues = detail::update( state, std::move( update.update), update.remove, update.zombies);
                           
                           state.multiplex.send( message.process.ipc, reply);
                        });

                        state.size.current = state.queuebase.size();
 
                        log::debug( "state: ", state);

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
                        log::debug( "message: ", message);

                        detail::perform( state);

                        log::debug( "state: ", state);
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

                        log::debug( "state.pending.dequeues: ", state.pending.dequeues);

                        auto transform_id = []( auto& value){ return value.queue;};
                        
                        // get available queues based on what we've got pending
                        auto available = state.queuebase.available( 
                           algorithm::transform( state.pending.dequeues, transform_id));
                        
                        log::debug( "available: ", available);

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
                                 
                                 // TODO this might send a reply, we need to fix this.
                                 // maybe using expected instead?
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
                              common::signal::timer::set( earliest_future->when - now);
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

            if( ! state.pending.replies.empty())
            {
               // persist the queuebase
               state.queuebase.persist();
               
               state.pending.replies.send( state.multiplex);
            }

            // handle pending dequeues, if any.
            local::detail::pending::dequeues( state);
         }

      } // handle

      handle::dispatch_type handlers( State& state)
      {
         return handle::dispatch_type{
            common::event::listener( 
               handle::local::dead::process( state),
               handle::local::dead::ipc( state)),
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

