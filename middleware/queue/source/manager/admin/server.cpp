//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/manager/admin/server.h"
#include "queue/manager/admin/services.h"
#include "queue/manager/transform.h"
#include "queue/manager/handle.h"

#include "queue/common/log.h"

#include "serviceframework/service/protocol.h"


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


            admin::model::State state( manager::State& state)
            {
               auto filter_running = []( auto& entities)
               {
                  return algorithm::filter( entities, []( auto& entity)
                  {
                     return entity.state == decltype( entity.state())::running;
                  });
               };

               auto group_states = algorithm::transform( filter_running( state.groups), [&]( auto& group)
               {
                  return communication::device::async::call( group.process.ipc, 
                     ipc::message::group::state::Request{ process::handle()});
               });

               auto forward_state = algorithm::transform( filter_running( state.forward.groups), [&]( auto& forward)
               {
                  return communication::device::async::call( forward.process.ipc, 
                     ipc::message::forward::group::state::Request{ process::handle()});
               });

               return transform::model::state( 
                  algorithm::transform( group_states, future_get),
                  algorithm::transform( forward_state, future_get));
            }

            namespace messages
            {
               std::vector< model::Message> list( const State& state, const std::string& queue)
               {
                  auto instance = state.queue( queue);

                  if( ! instance || instance->remote())
                     return {};

                  ipc::message::group::message::meta::Request request{ process::handle()};
                  request.qid = instance->queue;

                  auto reply = communication::ipc::call( instance->process.ipc, request);

                  return transform::model::message::meta( { std::move( reply)});
               }  

               std::vector< common::Uuid> remove( manager::State& state, const std::string& queue, std::vector< common::Uuid> ids)
               {
                  auto found = common::algorithm::find( state.queues, queue);

                  if( found && ! found->second.empty())
                  {
                     ipc::message::group::message::remove::Request request{ common::process::handle()};
                     request.queue = found->second.front().queue;
                     request.ids = std::move( ids);

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
                        if( auto instance = state.queue( name); instance && ! instance->remote())
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

               if( auto queue = state.queue( name))
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
                     log::line( verbose::log, "aliases: ", aliases);


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
                  return [&state]( common::service::invoke::Parameter&& parameter)
                  {
                     return serviceframework::service::user( 
                        serviceframework::service::protocol::deduce( std::move( parameter)), 
                        &local::state, state);
                  };
               };


               namespace messages
               {
                  auto list( manager::State& state)
                  {
                     return [&state]( common::service::invoke::Parameter&& parameter)
                     {
                        auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                        auto queue = protocol.extract< std::string>( "queue");

                        return serviceframework::service::user( 
                           std::move( protocol), 
                           &local::messages::list, 
                           state, 
                           std::move( queue));
                     };
                  }

                  auto remove( manager::State& state)
                  {
                     return [&state]( common::service::invoke::Parameter&& parameter)
                     {
                        auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                        auto queue = protocol.extract< std::string>( "queue");
                        auto ids = protocol.extract< std::vector< common::Uuid>>( "ids");

                        return serviceframework::service::user( 
                           std::move( protocol), 
                           &local::messages::remove, 
                           state, 
                           std::move( queue),
                           std::move( ids));
                     };
                  }
               } // messages

               auto restore( manager::State& state)
               {
                  return [&state]( common::service::invoke::Parameter&& parameter)
                  {
                     auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                     auto queue = protocol.extract< std::string>( "queue");

                     return serviceframework::service::user( std::move( protocol), &local::restore, state, queue);
                  };
               }

               auto clear( manager::State& state)
               {
                  return [&state]( common::service::invoke::Parameter&& parameter)
                  {
                     auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                     auto queues = protocol.extract< std::vector< std::string>>( "queues");

                     return serviceframework::service::user( std::move( protocol), &local::clear, state, std::move( queues));
                  };
               }

               namespace metric
               {
                  auto reset( manager::State& state)
                  {
                     return [&state]( common::service::invoke::Parameter&& parameter)
                     {
                        auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                        auto queues = protocol.extract< std::vector< std::string>>( "queues");

                        return serviceframework::service::user( std::move( protocol), &local::metric::reset, state, std::move( queues));
                     };
                  }
               } // metric

               namespace forward
               {
                  namespace scale
                  {
                     auto aliases( manager::State& state)
                     {
                        return [&state]( common::service::invoke::Parameter&& parameter)
                        {
                           auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                           auto aliases = protocol.extract< std::vector< manager::admin::model::scale::Alias>>( "aliases");

                           return serviceframework::service::user( std::move( protocol), &local::forward::scale::aliases, state, std::move( aliases));
                        };
                     }
                  } // metric
                  
               } // forward


            } // service
         } // <unnamed>
      } // local

      common::server::Arguments services( manager::State& state)
      {
         return { {
               { service::name::state,
                  local::service::state( state),
                  common::service::transaction::Type::none,
                  common::service::category::admin
               },
               { service::name::messages::list,
                  local::service::messages::list( state),
                  common::service::transaction::Type::none,
                  common::service::category::admin
               },
               { service::name::messages::remove,
                  local::service::messages::remove( state),
                  common::service::transaction::Type::none,
                  common::service::category::admin
               },
               { service::name::restore,
                  local::service::restore( state),
                  common::service::transaction::Type::none,
                  common::service::category::admin
               },
               { service::name::clear,
                  local::service::clear( state),
                  common::service::transaction::Type::none,
                  common::service::category::admin
               },
               { service::name::metric::reset,
                  local::service::metric::reset( state),
                  common::service::transaction::Type::none,
                  common::service::category::admin
               },
               { service::name::forward::scale::aliases,
                  local::service::forward::scale::aliases( state),
                  common::service::transaction::Type::none,
                  common::service::category::admin
               },

         }};
      }

   } // queue::manager::admin

} // casual
