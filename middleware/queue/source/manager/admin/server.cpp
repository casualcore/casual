//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/manager/admin/server.h"
#include "queue/manager/admin/services.h"
#include "queue/manager/state.h"
#include "queue/manager/transform.h"
#include "queue/manager/handle.h"

#include "queue/common/log.h"

#include "casual/manager/service/protocol.h"


namespace casual
{
   using namespace common;
   namespace queue::manager::admin
   {
      namespace local
      {
         namespace
         {
            auto future_get = []( auto& future){ return future.get( ipc::device());};

            namespace detail
            {
               namespace state
               {
                  template< typename Message>
                  auto send_request( State& state) 
                  {
                     return [ &state]( auto& group)
                     {
                        // If we fail to send, we just push an empty reply to our inbound device.
                        // This could happen later, if the group is busy and then shuts down (seems unlikely,
                        // but possible)
                        auto error_callback = []( auto&, auto& complete)
                        {
                           auto reply = common::message::reverse::type_t< Message>{};
                           reply.correlation = complete.correlation();
                           ipc::device().push( std::move( reply));
                        };

                        return state.multiplex.send( group.process.ipc, Message{ process::handle()}, std::move( error_callback));
                     };
                  }

                  using finalize_type = casual::manager::service::protocol::concurrent::Finalize< admin::model::State>;

                  struct Shared
                  {
                     Shared( manager::State& state, finalize_type finalize)
                        : state{ &state}, finalize{ std::move( finalize)}
                     {}

                     State* state;
                     finalize_type finalize;

                     std::vector< strong::correlation::id> correlations;

                     struct
                     {
                        std::vector< ipc::message::group::state::Reply> groups;
                        std::vector< ipc::message::forward::group::state::Reply> forwards;
                     } reply;

                     template< typename Message>
                     casual::task::unit::Dispatch try_finalize( Message&& message)
                     {
                        if( auto found = common::algorithm::find( correlations, message.correlation))
                        {
                           correlations.erase( std::begin( found));

                           if constexpr( std::is_same_v< std::decay_t< Message>, ipc::message::group::state::Reply>)
                              reply.groups.push_back( std::forward< Message>( message));
                           else
                              reply.forwards.push_back( std::forward< Message>( message));
                        }

                        if( correlations.empty())
                        {
                           finalize( transform::model::state( 
                              *state,
                              std::move( reply.groups),
                              std::move( reply.forwards)));

                           return casual::task::unit::Dispatch::done;
                        }

                        return casual::task::unit::Dispatch::pending;
                     }
                     
                  };
                  
               } // state
            } // detail

            void state( casual::manager::service::protocol::concurrent::Finalize< admin::model::State> finalize, manager::State& state)
            {
               Trace trace{ "queue::manager::admin::local::state"};

               // create the task unit that will handle the state request

               // shared state for the action and handlers
               auto shared = std::make_shared< detail::state::Shared>( state, std::move( finalize));

               // action that sends requests to all running groups
               auto action = casual::task::create::action( [ shared]( casual::task::unit::id)
               {
                  Trace trace{ "queue::manager::admin::local::state::action"};

                  auto is_running = []( auto& group)
                  {
                     return group.state == decltype( group.state())::running;
                  };

                  std::ranges::transform( std::views::filter( shared->state->groups, is_running),
                     std::back_inserter( shared->correlations),
                     detail::state::send_request< ipc::message::group::state::Request>( *shared->state));

                  std::ranges::transform( std::views::filter( shared->state->forward.groups, is_running),
                     std::back_inserter( shared->correlations),
                     detail::state::send_request< ipc::message::forward::group::state::Request>( *shared->state));
                  
                  // remove all 'invalid' correlations (ones that failed to send)
                  std::erase( shared->correlations, strong::correlation::id{});

                  if( shared->correlations.empty())
                     return casual::task::unit::action::Outcome::abort;

                  return casual::task::unit::action::Outcome::success;
               });

               auto handle_group_state = [ shared]( casual::task::unit::id, const ipc::message::group::state::Reply& message)
               {
                  Trace trace{ "queue::manager::admin::local::state::handle_group_state"};

                  return shared->try_finalize( message);
               };

               auto handle_forward_state = [ shared]( casual::task::unit::id, const ipc::message::forward::group::state::Reply& message)
               {
                  Trace trace{ "queue::manager::admin::local::state::handle_forward_state"};

                  return shared->try_finalize( message);
               };

               state.task.coordinator.then( casual::task::create::unit(
                  std::move( action),
                  std::move( handle_group_state),
                  std::move( handle_forward_state)));
            }


            namespace messages
            {
               std::vector< model::Message> list( const State& state, const std::string& queue)
               {
                  auto instance = state.queue( queue, {});

                  if( ! instance || instance->remote())
                     return {};

                  ipc::message::group::message::meta::Request request{ process::handle()};
                  request.qid = instance->queue;

                  auto reply = communication::ipc::call( instance->process.ipc, request);

                  return transform::model::message::meta( { std::move( reply)});
               }  

               std::vector< common::Uuid> remove( manager::State& state, const std::string& queue, std::vector< common::Uuid> ids, bool force)
               {
                  auto found = common::algorithm::find( state.queues, queue);

                  if( found && ! found->second.empty())
                  {
                     ipc::message::group::message::remove::Request request{ common::process::handle()};
                     request.queue = found->second.front().queue;
                     request.ids = std::move( ids);
                     request.force = force;

                     return communication::ipc::call( found->second.front().process.ipc, request).ids;
                  }

                  return {};
               }
            } // messages

            namespace detail
            {
               namespace local
               {
              
                  struct Instance 
                  {
                     strong::ipc::id ipc;
                     std::vector< strong::queue::id> queues;

                     friend bool operator == ( const Instance& lhs, const strong::ipc::id& rhs) { return lhs.ipc == rhs;}
                  };


                  std::vector< Instance> instances( const manager::State& state, const std::vector< std::string>& queues)
                  {
                     std::vector< Instance> result;

                     algorithm::for_each( queues, [&]( auto& name)
                     {
                        if( auto instance = state.queue( name, {}); instance && ! instance->remote())
                        {
                           if( auto found = algorithm::find( result, instance->process.ipc))
                              found->queues.push_back( instance->queue);
                           else
                              result.push_back( Instance{ instance->process.ipc, { instance->queue}});
                        }
                     });

                     return result;
                  }
               } // local
            } // detail


            std::vector< model::Affected> restore( manager::State& state, const std::string& name)
            {
               Trace trace{ "queue::manager::admin::local::restore"};
               
               std::vector< model::Affected> result;

               if( auto queue = state.queue( name, {}))
               {
                  ipc::message::group::queue::restore::Request request{ common::process::handle()};
                  request.queues.push_back( queue->queue);

                  auto reply = communication::ipc::call( queue->process.ipc, request);

                  if( ! reply.affected.empty())
                  {
                     auto& restored = reply.affected.front();
                     model::Affected affected;
                     affected.queue.name = restored.queue.name;
                     affected.queue.id = restored.queue.id;
                     affected.count = restored.count;

                     result.push_back( std::move( affected));
                  }
               }
               return result;
            }

            std::vector< model::Affected> clear( manager::State& state, const std::vector< std::string>& queues)
            {
               Trace trace{ "queue::manager::admin::local::clear"};

               auto clear_futures = algorithm::transform( detail::local::instances( state, queues), []( auto& instance)
               {
                  ipc::message::group::queue::clear::Request request{ process::handle()};
                  request.queues = std::move( instance.queues);
                  return communication::device::async::call( instance.ipc, request);
               });

               
               return algorithm::accumulate( clear_futures, std::vector< model::Affected>{}, []( auto result, auto& feature)
               {
                  auto transform = []( auto& value)
                  {
                     model::Affected result;
                     result.queue.id = value.queue.id;
                     result.queue.name = value.queue.name;
                     result.count = value.count;
                     return result;
                  };

                  algorithm::transform( future_get( feature).affected, std::back_inserter( result), transform);
                  return result;
               });

            }

            std::vector< common::transaction::global::ID> recover( manager::State& state, const std::vector< common::transaction::global::ID>& gtrids, 
               ipc::message::group::message::recovery::Directive directive)
            {
               Trace trace{ "queue::manager::admin::local::recover"};

               auto recover_futures = algorithm::transform( state.groups, [&gtrids, directive]( auto& group)
               {
                  ipc::message::group::message::recovery::Request request{ process::handle()};
                  request.gtrids = gtrids;
                  request.directive = directive;
                  return communication::device::async::call( group.process.ipc, request);
               });

               return algorithm::accumulate( recover_futures, std::vector< common::transaction::global::ID>{}, []( auto result, auto& feature)
               {
                  auto predicate = [&result]( auto& value)
                  {
                     return ! algorithm::find( result, value);
                  };

                  algorithm::copy_if( future_get( feature).gtrids, std::back_inserter( result), predicate);
                  return result;
               });
            }

            namespace metric
            {
               void reset( manager::State& state, const std::vector< std::string>& queues)
               {
                  Trace trace{ "queue::manager::admin::local::metric::reset"};

                  auto reset_futures = algorithm::transform( detail::local::instances( state, queues), []( auto& instance)
                  {
                     ipc::message::group::metric::reset::Request request{ process::handle()};
                     request.queues = std::move( instance.queues);
                     return communication::device::async::call( instance.ipc, request);
                  });

                  algorithm::for_each( reset_futures, future_get);
               }
               
            } // metric

            namespace forward
            {
               namespace scale
               {
                  void aliases( State& state, const std::vector< manager::admin::model::scale::Alias>& aliases)
                  {
                     Trace trace{ "queue::manager::admin::local::forward::scale::aliases"};
                     log::debug( "aliases: ", aliases);


                     auto origin = state.forward.groups;

                     // update the configuration
                     algorithm::for_each( state.forward.groups, [&aliases]( auto& forward)
                     {
                        // update queue or service instansces, if alias is found
                        auto update_instances = [&aliases]( auto& instance)
                        {
                           auto is_alias = [&instance]( auto& alias)
                           {
                              return instance.alias == alias.name;
                           };

                           if( auto found = algorithm::find_if( aliases, is_alias))
                              instance.instances = found->instances;
                        };

                        algorithm::for_each( forward.configuration.services, update_instances);
                        algorithm::for_each( forward.configuration.queues, update_instances);
                     });

                     auto updated  = std::get< 1>( algorithm::intersection( state.forward.groups, origin));

                     auto features = algorithm::transform( updated, []( auto& forward)
                     {
                        ipc::message::forward::group::configuration::update::Request request{ process::handle()};
                        request.model = forward.configuration;
                        return communication::device::async::call( forward.process.ipc, request);
                     });

                     algorithm::for_each( features, future_get);

                  }
               } // scale
            } // forward


            namespace service
            {
               auto state( manager::State& state)
               {
                  return [&state]( casual::manager::service::invoke::concurrent::Parameter&& parameter)
                  {
                     auto concurrent = casual::manager::service::protocol::deduce( std::move( parameter));

                     return casual::manager::service::protocol::concurrent::dispatch( 
                        std::move( concurrent), 
                        &local::state, 
                        state);
                  };
               };


               namespace messages
               {
                  auto list( manager::State& state)
                  {
                     return [&state]( casual::manager::service::invoke::Parameter&& parameter)
                     {
                        auto protocol = casual::manager::service::protocol::deduce( std::move( parameter));

                        auto queue = protocol.extract< std::string>( "queue");

                        return casual::manager::service::protocol::dispatch( 
                           std::move( protocol), 
                           &local::messages::list, 
                           state, 
                           std::move( queue));
                     };
                  }

                  auto remove( manager::State& state)
                  {
                     return [&state]( casual::manager::service::invoke::Parameter&& parameter)
                     {
                        auto protocol = casual::manager::service::protocol::deduce( std::move( parameter));

                        auto queue = protocol.extract< std::string>( "queue");
                        auto ids = protocol.extract< std::vector< common::Uuid>>( "ids");
                        auto force = protocol.extract< bool>( "force");

                        return casual::manager::service::protocol::dispatch( 
                           std::move( protocol), 
                           &local::messages::remove, 
                           state, 
                           std::move( queue),
                           std::move( ids),
                           force);
                     };
                  }
               } // messages

               auto restore( manager::State& state)
               {
                  return [&state]( casual::manager::service::invoke::Parameter&& parameter)
                  {
                     auto protocol = casual::manager::service::protocol::deduce( std::move( parameter));

                     auto queue = protocol.extract< std::string>( "queue");

                     return casual::manager::service::protocol::dispatch( std::move( protocol), &local::restore, state, queue);
                  };
               }

               auto clear( manager::State& state)
               {
                  return [&state]( casual::manager::service::invoke::Parameter&& parameter)
                  {
                     auto protocol = casual::manager::service::protocol::deduce( std::move( parameter));

                     auto queues = protocol.extract< std::vector< std::string>>( "queues");

                     return casual::manager::service::protocol::dispatch( std::move( protocol), &local::clear, state, std::move( queues));
                  };
               }

               auto recover( manager::State& state)
               {
                  return [&state]( casual::manager::service::invoke::Parameter&& parameter)
                  {
                     auto protocol = casual::manager::service::protocol::deduce( std::move( parameter));

                     auto gtrids = protocol.extract< std::vector< common::transaction::global::ID>>( "gtrids");
                     using Directive = ipc::message::group::message::recovery::Directive;
                     auto directive = protocol.extract< Directive>("directive");

                     return casual::manager::service::protocol::dispatch( std::move( protocol), &local::recover, state, std::move( gtrids), directive);
                  };
               }

               namespace metric
               {
                  auto reset( manager::State& state)
                  {
                     return [&state]( casual::manager::service::invoke::Parameter&& parameter)
                     {
                        auto protocol = casual::manager::service::protocol::deduce( std::move( parameter));

                        auto queues = protocol.extract< std::vector< std::string>>( "queues");

                        return casual::manager::service::protocol::dispatch( std::move( protocol), &local::metric::reset, state, std::move( queues));
                     };
                  }
               } // metric

               namespace forward
               {
                  namespace scale
                  {
                     auto aliases( manager::State& state)
                     {
                        return [&state]( casual::manager::service::invoke::Parameter&& parameter)
                        {
                           auto protocol = casual::manager::service::protocol::deduce( std::move( parameter));

                           auto aliases = protocol.extract< std::vector< manager::admin::model::scale::Alias>>( "aliases");

                           return casual::manager::service::protocol::dispatch( std::move( protocol), &local::forward::scale::aliases, state, std::move( aliases));
                        };
                     }
                  } // metric
                  
               } // forward


            } // service
         } // <unnamed>
      } // local

      std::vector< casual::manager::Service> services( manager::State& state)
      {
         return { 
            casual::manager::concurrent::Service{ .name = std::string{ service::name::state},
               .function =local::service::state( state),
               .visibility = common::service::visibility::Type::undiscoverable,
               .category = std::string{ common::service::category::admin}
            },
            casual::manager::sequential::Service{ .name = std::string{ service::name::messages::list},
               .function =local::service::messages::list( state),
               .visibility = common::service::visibility::Type::undiscoverable,
               .category = std::string{ common::service::category::admin}
            },
            casual::manager::sequential::Service{ .name = std::string{ service::name::messages::remove},
               .function =local::service::messages::remove( state),
               .visibility = common::service::visibility::Type::undiscoverable,
               .category = std::string{ common::service::category::admin}
            },
            casual::manager::sequential::Service{ .name = std::string{ service::name::restore},
               .function =local::service::restore( state),
               .visibility = common::service::visibility::Type::undiscoverable,
               .category = std::string{ common::service::category::admin}
            },
            casual::manager::sequential::Service{ .name = std::string{ service::name::clear},
               .function =local::service::clear( state),
               .visibility = common::service::visibility::Type::undiscoverable,
               .category = std::string{ common::service::category::admin}
            },
            casual::manager::sequential::Service{ .name = std::string{ service::name::recover},
               .function =local::service::recover( state),
               .visibility = common::service::visibility::Type::undiscoverable,
               .category = std::string{ common::service::category::admin}
            },
            casual::manager::sequential::Service{ .name = std::string{ service::name::metric::reset},
               .function =local::service::metric::reset( state),
               .visibility = common::service::visibility::Type::undiscoverable,
               .category = std::string{ common::service::category::admin}
            },
            casual::manager::sequential::Service{ .name = std::string{ service::name::forward::scale::aliases},
               .function =local::service::forward::scale::aliases( state),
               .visibility = common::service::visibility::Type::undiscoverable,
               .category = std::string{ common::service::category::admin}
            }
         };
      }

   } // queue::manager::admin

} // casual
