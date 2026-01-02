//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/fanout/handle.h"
#include "queue/common/log.h"

#include "common/message/dispatch/handle.h"
#include "common/message/transaction.h"
#include "common/log.h"
#include "common/communication/instance.h"

#include "configuration/model/change.h"

#include "casual/overloaded.h"


namespace casual
{
   using namespace common;

   namespace queue::fanout::handle
   {
      namespace local
      {
         namespace
         {
            state::machine::rollback send_rollback( State& state, auto state_machine)
            {
               Trace trace{ "queue::fanout::handle::local::send_rollback"};

               auto rollback = state::machine::rollback{};
               rollback.trid = state_machine.trid;

               // send rollback message to TM
               {
                  message::transaction::rollback::Request message{ common::process::handle()};
                  message.trid = rollback.trid;

                  rollback.correlation = state.multiplex.send( communication::instance::outbound::transaction::manager::device(), message);
               }
               
              
               return rollback;
            }

            state::machine::source_lookup start_flow( State& state, state::profile::id id)
            {
               Trace trace{ "queue::fanout::handle::local::start_flow"};

               state::machine::source_lookup result;
               result.trid = common::transaction::id::create();

               const auto& profile = state.profiles[ id];

               // lookup source queue
               {
                  ipc::message::lookup::Request request{ common::process::handle()};
                  request.name = profile.configuration.source;
                  request.gtrid = common::transaction::global::ID{ result.trid.global()};
                  request.context.semantic = decltype( request.context.semantic)::wait;
                  request.context.action = decltype( request.context.action)::dequeue;

                  result.correlation = state.multiplex.send( communication::instance::outbound::queue::manager::device(), request);
               }
               
               return result;
            }

            namespace configuration::update
            {
               namespace detail
               {
                  state::profile::id id_from_alias( const auto& lookup, std::string_view alias)
                  {
                     for( auto index : lookup.indexes())
                        if( lookup[ index].configuration.alias == alias)
                           return index;

                     return {};
                  }

                  void send_forget_dequeue( State& state, const state::machine::source_dequeue& machine)
                  {
                     Trace trace{ "queue::fanout::handle::configuration::update::detail::send_forget_dequeue"};

                     ipc::message::group::dequeue::forget::Request request{ common::process::handle()};
                     request.correlation = machine.correlation;
                     request.name = machine.queue.name;
                     request.queue = machine.queue.id;

                     state.multiplex.send( machine.queue.group.ipc, request);
                  }  

                  auto stop_instance( State& state, state::Instance& instance)
                  {
                     Trace trace{ "queue::fanout::handle::configuration::update::detail::stop_instance"};
                     log::debug( "stopping instance: ", instance);

                     // disassociate instance from configuration
                     instance.profile = {};
                     instance.life = state::instance::Life::stop;

                     auto send_lookup_discard = [ &state]( auto& correlation)
                     {
                        ipc::message::lookup::discard::Request request{ common::process::handle()};
                        request.correlation = correlation;
                        state.multiplex.send( communication::instance::outbound::queue::manager::device(), request);
                     };

                     auto visit = casual::overloaded{
                        [&]( state::machine::source_lookup& machine)
                        {
                           // we need to send lookup discard to QM
                           send_lookup_discard( machine.correlation);
                           instance.state_machine = state::machine::source_lookup_discard{ .correlation = machine.correlation};
                        },
                        [&]( state::machine::targets_lookup& machine)
                        {
                           // we need to send lookup discard to QM for all pending targets
                           std::ranges::for_each( machine.correlations, send_lookup_discard);
                           instance.state_machine = state::machine::targets_lookup_discard{ .correlations = std::move( machine.correlations)};
                        },
                        [&]( state::machine::source_dequeue& machine)
                        {
                           // we need to discard the dequeue
                           send_forget_dequeue( state, machine);
                        },
                        [&]( auto& machine)
                        {
                           log::debug( "visit stop_instance - no action for: ", typeid( machine).name(), " - ",  machine);
                        }
                     };

                     std::visit( visit, instance.state_machine);
                  }

                  auto add_instance( State& state, state::profile::id id)
                  {
                     Trace trace{ "queue::fanout::handle::configuration::update::detail::add_instance"};

                     state::Instance instance;
                     instance.profile = id;
                     instance.state_machine = start_flow( state, id);

                     state.instances.push_back( std::move( instance));
                  }

                  auto filter_instances( std::span< state::Instance> instances, state::profile::id id)
                  {
                     auto is_instance = [ id]( const auto& instance) 
                     {
                        return instance.profile == id
                           // we're only interested in running instances (not instances marked for stop)
                           && instance.life == state::instance::Life::restart;
                     };
                     return algorithm::filter( instances, is_instance);
                  }

                  void remove( State& state, auto removed)
                  {
                     Trace trace{ "queue::fanout::handle::configuration::update::remove"};
                     log::debug( "removed: ", removed);

                     auto prepare_remove = [ &state]( auto& configuration)
                     {
                        if( auto id = id_from_alias( state.profiles, configuration.alias))
                        {
                           state.profiles.erase( id);

                           auto instances = filter_instances( state.instances, id);

                           for( auto& instance : instances)
                              stop_instance( state, instance);
                        }
                        else
                          log::error( code::casual::invalid_argument, "cannot remove configuration for fanout queue: ", configuration.alias, " - not found");
                     };

                     std::ranges::for_each( removed, prepare_remove);
                  }

                  void add( State& state, auto added)
                  {
                     Trace trace{ "queue::fanout::handle::configuration::update::add"};
                     log::debug( "added: ", added);

                     auto prepare_add = [ &state]( auto& configuration)
                     {
                        auto id = state.profiles.insert( state::Profile{ .configuration = configuration});

                        algorithm::for_n( configuration.effective_instances(), [&state, id]( auto& instance)
                        {
                           add_instance( state, id);
                        });
                     };

                     std::ranges::for_each( added, prepare_add);
                  }

                  void update( State& state, auto updated)
                  {
                     Trace trace{ "queue::fanout::handle::configuration::update::detail::update"};
                     log::debug( "updated: ", updated);

                     auto prepare_update = [ &state]( auto& configuration)
                     {
                        if( auto id = id_from_alias( state.profiles, configuration.alias))
                        {
                           state.profiles[ id].configuration = configuration;

                           auto instances = filter_instances( state.instances, id);

                           auto instance_count = configuration.effective_instances();

                           if( std::ssize( instances) < instance_count)
                           {
                              // need to add instances
                              auto add_count = instance_count - std::size( instances);
                              for( auto id : std::views::repeat( id, add_count))
                                 add_instance( state, id);
                           }
                           else if( std::ssize( instances) > instance_count)
                           {
                              // need to remove instances
                              auto stop_count = std::size( instances) - instance_count;

                              for( auto& instance : instances | std::views::take( stop_count))
                                 stop_instance( state, instance);
                           }
                        }
                        else
                           log::error( code::casual::invalid_argument, "cannot update configuration for fanout queue: ", configuration.alias, " - not found");
                     };

                     std::ranges::for_each( updated, prepare_update);
                  }

               } // detail

               auto comply( State& state, std::vector< casual::configuration::model::queue::fanout::Queue> wanted)
               {
                  Trace trace{ "queue::fanout::handle::configuration::update::comply"};

                  auto current = state.configuration_model();
                  auto change = casual::configuration::model::change::calculate( current.queues, wanted);

                  detail::remove( state, change.removed);
                  detail::update( state, change.modified);
                  detail::add( state, change.added);
               }

               auto request( State& state)
               {
                  return [&state]( ipc::message::fanout::group::configuration::update::Request&& message)
                  {
                     Trace trace{ "queue::fanout::handle::configuration::update::request"};
                     log::debug( "message: ", message);

                     state.configuration.alias = message.model.alias;
                     state.configuration.note = message.model.note;
                     state.configuration.memberships = message.model.memberships;

                     comply( state, std::move( message.model.queues));

                     auto reply = common::message::reverse::type( message, common::process::handle());
                     reply.alias = message.model.alias;
                     state.multiplex.send( message.process.ipc, reply);
                  };
               }
            } // configuration::update

            namespace lookup
            {
               namespace detail
               {
                  state::state_machine_type dequeue( State& state, state::machine::source_lookup lookup, const ipc::message::lookup::Reply& source)
                  {
                     Trace trace{ "queue::fanout::handle::lookup::detail::dequeue"};

                     // if source is not available, we start rollback
                     if( ! source)
                        return send_rollback( state, std::move( lookup));

                     state::machine::source_dequeue result;
                     result.correlation = lookup.correlation;
                     result.trid = lookup.trid;
                     result.queue.id = source.queue;
                     result.queue.name = source.name;
                     result.queue.group = source.process;

                     // request dequeue from source
                     {
                        ipc::message::group::dequeue::Request request{ common::process::handle()};
                        request.correlation = result.correlation;
                        request.queue = source.queue;
                        // wait for a available message
                        request.block = true;
                        request.name = source.name;
                        request.trid = lookup.trid;

                        state.multiplex.send( source.process.ipc, request);
                     }

                     return result;
                  }

                  state::state_machine_type enqueue( State& state, state::machine::targets_lookup lookup, state::profile::id id)
                  {
                     Trace trace{ "queue::fanout::handle::lookup::detail::enqueue"};

                     // first we check if any of the targets are not available, if so, we start rollback
                     if( algorithm::any_of( lookup.replies, []( const auto& reply){ return ! reply;}) )
                        return send_rollback( state, std::move( lookup));


                     auto create_request = []( auto& lookup)
                     {
                        ipc::message::group::enqueue::Request request{ common::process::handle()};
                        request.message = ipc::message::group::enqueue::Message{
                           .attributes = std::move( lookup.message.message->attributes),
                           .payload = std::move( lookup.message.message->payload)
                        };
                        request.trid = lookup.trid;
                        return request;
                     };

                     auto enqueue = [ &state, id, request = create_request( lookup)]( auto& target) mutable
                     {
                        const auto& profile = state.profiles[ id];

                        request.queue = target.queue;
                        request.name = target.name;
                        // generate new id for the message
                        request.message.id = uuid::make();

                        // should we add delay?
                        if( auto found = algorithm::find( profile.configuration.targets, target.name))
                           if( found->delay > std::chrono::seconds::zero())
                              request.message.attributes.available = common::chronology::time_point::clock::now() + found->delay;

                        return state.multiplex.send( target.process.ipc, request);
                     };

                     state::machine::targets_enqueue result;
                     result.trid = lookup.trid;
                     result.correlations = algorithm::transform( lookup.replies, enqueue);
                     return result;
                  }
                  
               } // detail

               auto reply( State& state)
               {
                  return [ &state]( const ipc::message::lookup::Reply& message)
                  {
                     Trace trace{ "queue::fanout::handle::lookup::reply"};
                     log::debug( "message: ", message);

                     if( auto found = algorithm::find( state.instances, message.correlation))
                     {
                        if( auto instance = std::get_if< state::machine::source_lookup>( &found->state_machine))
                        {
                           found->state_machine = detail::dequeue( state, std::move( *instance), message);
                        }
                        else if( auto instance = std::get_if< state::machine::targets_lookup>( &found->state_machine))
                        {
                           std::erase( instance->correlations, message.correlation);
                           instance->replies.push_back( message);

                           // if we're ready to enqueue to targets, we do it and update the state-machine
                           if( instance->correlations.empty())
                              found->state_machine = detail::enqueue( state, std::move( *instance), found->profile);
                        }
                        else
                           log::error( code::casual::invalid_semantics, "unexpected state in state-machine: ", found->state_machine, " - action: discard");
                     }
                     else
                        log::debug( "no instance found for correlation: ", message.correlation);                 

                  };
               }

               namespace discard
               {
                  auto reply( State& state)
                  {
                     return [&state]( const ipc::message::lookup::discard::Reply& message)
                     {
                        Trace trace{ "queue::fanout::handle::lookup::discard::reply"};
                        log::debug( "message: ", message);

                        if( auto found = algorithm::find( state.instances, message.correlation))
                        {
                           if( std::get_if< state::machine::source_lookup_discard>( &found->state_machine))
                           {
                              // we're done discarding and in a safe state, should we restart or stop the instance?
                              if( found->life == decltype( found->life)::stop)
                                 algorithm::container::erase( state.instances, std::begin( found));
                              else
                                 found->state_machine = start_flow( state, found->profile);

                           }
                           else if( auto instance = std::get_if< state::machine::targets_lookup_discard>( &found->state_machine))
                           {
                              std::erase( instance->correlations, message.correlation);

                              // are we done discarding?
                              if( ! instance->correlations.empty())
                                 return;

                              // we have dequeued a message in transaction, we need to rollback.
                              found->state_machine = send_rollback( state, std::move( *instance));
                           }
                           else
                              log::error( code::casual::invalid_semantics, "unexpected state in state-machine: ", found->state_machine, " - action: discard");
                        }                     
                     };
                  }
                  
               } // discard
               
            } // lookup

            namespace dequeue
            {
               namespace detail
               {
                   state::state_machine_type lookup( State& state, state::machine::source_dequeue dequeue, ipc::message::group::dequeue::Reply message, state::profile::id id)
                  {
                     Trace trace{ "queue::fanout::handle::dequeue::detail::lookup"};

                     // if something went wrong during dequeue, we rollback
                     if( ! message)
                        return send_rollback( state, std::move( dequeue));

                     // if the configuration is missing, we rollback
                     if( ! state.profiles.contains( id))
                        return send_rollback( state, std::move( dequeue));

                     const auto& profile = state.profiles[ id];

                     state::machine::targets_lookup result;
                     result.message = std::move( message);
                     result.trid = dequeue.trid;

                     auto lookup_target = [ &]( auto& target)
                     {
                        ipc::message::lookup::Request request{ common::process::handle()};
                        request.name = target.queue;
                        request.gtrid = common::transaction::global::ID{ dequeue.trid.global()};
                        request.context.semantic = decltype( request.context.semantic)::wait;
                        request.context.action = decltype( request.context.action)::enqueue;

                        return state.multiplex.send( communication::instance::outbound::queue::manager::device(), request);
                     };

                     result.correlations = algorithm::transform( profile.configuration.targets, lookup_target);
                     
                     return result;
                  }
                  
               } // detail

               auto reply( State& state)
               {
                  return [ &state]( ipc::message::group::dequeue::Reply&& message)
                  {
                     Trace trace{ "queue::fanout::handle::dequeue::reply"};
                     log::debug( "message: ", message);

                     if( auto found = algorithm::find( state.instances, message.correlation))
                     {
                        if( auto instance = std::get_if< state::machine::source_dequeue>( &found->state_machine))
                           found->state_machine = detail::lookup( state, std::move( *instance), std::move( message), found->profile);
                        else
                           log::error( code::casual::invalid_semantics, "unexpected state in state-machine: ", found->state_machine, " - action: discard");
                     }
                     else
                        log::debug( "no instance found for correlation: ", message.correlation);                 
                  };
               }
            } // dequeue

            namespace enqueue
            {
               namespace detail
               {
                  state::state_machine_type commit( State& state, state::machine::targets_enqueue enqueue)
                  {
                     Trace trace{ "queue::fanout::handle::enqueue::detail::commit"};
                     log::debug( "enqueue: ", enqueue);

                     // if all targets enqueued ok, we commit the transaction
                     if( algorithm::any_of( enqueue.replies, []( const auto& reply){ return reply.code != common::code::queue::ok;}))
                        return local::send_rollback( state, enqueue);

                     // otherwise, we commit
                     state::machine::commit result;
                     result.trid = enqueue.trid;

                     {
                        message::transaction::commit::Request request{ common::process::handle()};
                        request.trid = enqueue.trid;

                        result.correlation = state.multiplex.send( communication::instance::outbound::transaction::manager::device(), request);
                     }                     

                     return result;
                  }
                  
               } // detail


               auto reply( State& state)
               {
                  return [ &state]( ipc::message::group::enqueue::Reply&& message)
                  {
                     Trace trace{ "queue::fanout::handle::enqueue::reply"};
                     log::debug( "message: ", message);

                     if( auto found = algorithm::find( state.instances, message.correlation))
                     {
                        if( auto instance = std::get_if< state::machine::targets_enqueue>( &found->state_machine))
                        {
                           std::erase( instance->correlations, message.correlation);
                           instance->replies.push_back( std::move( message));

                           // if we have all replies, we can consider the 'flow' done
                           if( instance->correlations.empty())
                              found->state_machine = detail::commit( state, std::move( *instance));
                           
                        }
                     }
                     else
                        log::debug( "no instance found for correlation: ", message.correlation);
                  };
               }
            } // enqueue

            namespace transaction
            {
               auto commit_reply( State& state)
               {
                  return [ &state]( const message::transaction::commit::Reply& message)
                  {
                     Trace trace{ "queue::fanout::handle::transaction::commit_reply"};
                     log::debug( "message: ", message);

                     if( auto found = algorithm::find( state.instances, message.correlation))
                     {
                        if( std::get_if< state::machine::commit>( &found->state_machine))
                        {
                           auto& profile = state.profiles[ found->profile];
                           profile.metric.commit.count += 1;
                           profile.metric.commit.last = common::chronology::time_point::clock::now();

                           // we're done with this 'flow', should we restart or stop the instance?
                           if( found->life == decltype( found->life)::stop)
                              algorithm::container::erase( state.instances, std::begin( found));
                           else
                              found->state_machine = start_flow( state, found->profile);
                        }
                        else
                           log::error( code::casual::invalid_semantics, "unexpected state in state-machine: ", found->state_machine, " - action: discard");
                     }
                  };
               }

               auto rollback_reply( State& state)
               {
                  return [ &state]( const message::transaction::rollback::Reply& message)
                  {
                     Trace trace{ "queue::fanout::handle::transaction::rollback_reply"};
                     log::debug( "message: ", message);

                     if( auto found = algorithm::find( state.instances, message.correlation))
                     {
                        if( std::get_if< state::machine::rollback>( &found->state_machine))
                        {
                           auto& profile = state.profiles[ found->profile];
                           profile.metric.rollback.count += 1;
                           profile.metric.rollback.last = common::chronology::time_point::clock::now();

                           // we're done with this 'flow', should we restart or stop the instance?
                           if( found->life == decltype( found->life)::stop)
                              algorithm::container::erase( state.instances, std::begin( found));
                           else
                              found->state_machine = start_flow( state, found->profile);
                        }
                        else
                           log::error( code::casual::invalid_semantics, "unexpected state in state-machine: ", found->state_machine, " - action: discard");
                     }
                  };
               }
               
            } // transaction

            namespace dequeue::forget
            {
               auto reply( State& state)
               {
                  return [ &state]( const ipc::message::group::dequeue::forget::Reply& message)
                  {
                     Trace trace{ "queue::fanout::handle::dequeue::forget::reply"};
                     log::debug( "message: ", message);

                     if( auto found = algorithm::find( state.instances, message.correlation))
                     {
                        if( std::get_if< state::machine::source_dequeue>( &found->state_machine))
                        {
                           log::debug( "state_machine: ", found->state_machine);

                           // QM could have dequeue the message, if so we let the flow continue
                           // The dequeue message should have reach us before, but then we should be 
                           // in the state::machine::targets_lookup state
                           if( ! message.discarded)
                             return;

                           // we're done discarding and in a safe state. We don't need to rollback since the
                           // dequeue was never performed. Should we restart or stop the instance?
                           // We should always be in the 'stop' state here, but... just to be sure...
                           if( found->life == decltype( found->life)::stop)
                              algorithm::container::erase( state.instances, std::begin( found));
                           else
                              found->state_machine = start_flow( state, found->profile);
                        }
                        else if( std::get_if< state::machine::targets_lookup>( &found->state_machine))
                        {
                           // QM could have dequeue the message, if so we let the flow continue
                           log::debug( "targets_lookup - let the flow continue: ", found->state_machine);
                        }
                        else
                           log::error( code::casual::invalid_semantics, "unexpected state in state-machine: ", found->state_machine, " - action: discard");
                     }
                     else
                        log::debug( "no instance found for correlation: ", message.correlation);                   
                  };
               }

               auto request( State& state)
               {
                  return [&state]( const ipc::message::group::dequeue::forget::Request& message)
                  {
                     Trace trace{ "queue::fanout::handle::dequeue::forget::request"};
                     log::debug( "message: ", message);

                     if( auto found = algorithm::find( state.instances, message.correlation))
                     {
                        if( std::get_if< state::machine::source_dequeue>( &found->state_machine))
                        {
                           auto reply = common::message::reverse::type( message);
                           reply.discarded = true;
                           state.multiplex.send( message.process.ipc, reply);

                           if( found->life == decltype( found->life)::stop)
                              algorithm::container::erase( state.instances, std::begin( found));
                           else
                              found->state_machine = start_flow( state, found->profile);

                        }
                        else
                           log::error( code::casual::invalid_semantics, "unexpected state in state-machine: ", found->state_machine, " - action: discard");
                     }
                  };
               }

            } // dequeue::forget

            namespace runtime::state
            {
               namespace detail
               {
                  auto transform_queues( const State& state)
                  {
                     return algorithm::transform( state.profiles.indexes(), [ &state]( auto id)
                     {
                        const auto& profile = state.profiles[ id];

                        auto transform_metric = []( const auto& metric)
                        {
                           ipc::message::fanout::group::state::Reply::Metric result;
                           result.commit.count = metric.commit.count;
                           result.commit.last = metric.commit.last;
                           result.rollback.count = metric.rollback.count;
                           result.rollback.last = metric.rollback.last;
                           return result;
                        };

                        auto transform_target = []( const auto& target)
                        {
                           ipc::message::fanout::group::state::Reply::Target result;
                           result.queue = target.queue;
                           result.delay = target.delay;
                           return result;
                        };

                        auto accumulate_instances = [ id]( std::tuple< platform::size::type, platform::size::type> count, const auto& instance)
                        {
                           if( instance.profile != id)
                              return count;
                           if( instance.life == decltype( instance.life)::stop)
                              std::get< 1>( count)++;
                           if( instance.life == decltype( instance.life)::restart)
                              std::get< 0>( count)++;
                           return count;
                        };

                        ipc::message::fanout::group::state::Reply::Queue result;
                        result.alias = profile.configuration.alias;
                        result.source = profile.configuration.source;
                        result.targets = algorithm::transform( profile.configuration.targets, transform_target);
                        result.metric = transform_metric( profile.metric);
                        result.note = profile.configuration.note;
                        result.enabled = profile.configuration.enabled;

                        result.instances.configured = profile.configuration.instances;
                        std::tie( result.instances.running, result.instances.stopped) = 
                           algorithm::accumulate( state.instances, std::tuple< platform::size::type, platform::size::type>{}, accumulate_instances);

                        return result;
                     });
                  }
                  
               } // detail

               auto request( State& state)
               {
                  return [ &state]( const ipc::message::fanout::group::state::Request& message)
                  {
                     Trace trace{ "queue::fanout::handle::state::request"};
                     log::debug( "message: ", message);

                     auto reply = common::message::reverse::type( message, common::process::handle());

                     reply.alias = state.configuration.alias;
                     reply.note = state.configuration.note;
                     reply.queues = detail::transform_queues( state);


                     state.multiplex.send( message.process.ipc, reply);
                  };
               }
            } // runtime::state

            namespace shutdown
            {
               auto request( State& state)
               {
                  return [&state]( const common::message::shutdown::Request& message)
                  {
                     Trace trace{ "queue::fanout::handle::shutdown::request"};
                     log::debug( "message: ", message);

                     state.runlevel = decltype( state.runlevel())::shutdown;

                     // we try to comply with an empty configuration
                     local::configuration::update::comply( state, {});
                  };
               }
            } // shutdown

         } // <unnamed>
      } // local

      void abort( State& state)
      {
         Trace trace{ "queue::fanout::handle::abort"};

         log::error( code::casual::fatal_terminate, "abort requested - best effort to stop all instances");

         auto try_to_stop_instance = [ &state]( auto& instance)
         {
            // mark for stop, and possible send discards.
            local::configuration::update::detail::stop_instance( state, instance);

            // instance could be midflight in a transaction, we try to rollback
            auto visit = casual::overloaded{
               [&]( state::machine::source_dequeue&& machine) -> state::state_machine_type
               {
                  // we've requested a dequeue in transaction. stop_instance will send _forget_.
                  // But to be safe we rollback.
                  return local::send_rollback( state, std::move( machine));
               },
               [&]( state::machine::targets_lookup&& machine) -> state::state_machine_type
               {
                  // we have dequeued a message in transaction, we need to rollback.
                  return local::send_rollback( state, std::move( machine));
               },
               [&]( state::machine::targets_enqueue&& machine) -> state::state_machine_type
               {
                  // we have dequeued a message, and possible enqueued to some targets in transaction, we need to rollback.
                  return local::send_rollback( state, std::move( machine));
               },
               [&]( auto&& machine) -> state::state_machine_type
               {
                  // nothing to do, hope for the best
                  return std::move( machine);
               }
            };
            instance.state_machine = std::visit( visit, std::move( instance.state_machine));
         };

         std::ranges::for_each( state.instances, try_to_stop_instance);
      }


      handler_type create( State& state)
      {
         return handler_type{
            common::message::dispatch::handle::defaults( state),
            local::configuration::update::request( state),
            local::lookup::reply( state),
            local::lookup::discard::reply( state),
            local::dequeue::reply( state),
            local::dequeue::forget::request( state),
            local::dequeue::forget::reply( state),
            local::enqueue::reply( state),
            local::transaction::commit_reply( state),
            local::transaction::rollback_reply( state),
            local::shutdown::request( state),
            local::runtime::state::request( state),
         };
      }

   } // queue::fanout::handle
} // casual
