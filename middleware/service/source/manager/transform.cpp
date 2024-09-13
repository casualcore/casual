//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "service/manager/transform.h"
#include "service/common.h"

#include "common/environment.h"
#include "common/chronology.h"
#include "common/predicate.h"

namespace casual
{
   namespace service::manager::transform
   {
      namespace local
      {
         namespace
         {
            auto pending()
            {
               return []( auto& value)
               {
                  manager::admin::model::Pending result;
                  result.process = value.request.process;
                  result.requested = value.request.requested;
                  return result;
               };
            };

            void services( const manager::State& state, auto& target)
            {
               state.services.for_each( [ &state, &target]( auto id, auto& name, auto& service)
               {
                  auto transform_metric = []( const auto& value)
                  {
                     manager::admin::model::Metric result;
                     result.count = value.count;
                     result.total = value.total;
                     result.limit.min = value.limit.min;
                     result.limit.max = value.limit.max;
                     return result;
                  };

                  manager::admin::model::Service result;

                  result.name = name;
                  result.execution.timeout = service.timeout;
                  if( auto found = common::algorithm::find( state.services.metrics(), name))
                  {
                     result.metric.invoked = transform_metric( found->second.invoked);
                     result.metric.pending = transform_metric( found->second.pending);
                     result.metric.remote = found->second.remote;
                     result.metric.last = found->second.last;
                  }
                  result.category = service.category;
                  result.transaction = service.transaction;
                  result.visibility = service.visibility.value_or( common::service::visibility::Type::discoverable);
                  

                  auto transform_concurrent = [ &state]( auto concurrent)
                  {
                     auto& instance = state.instances.concurrent[ concurrent.id];
                     return manager::admin::model::service::instance::Concurrent{
                        instance.process,
                        concurrent.hops
                     };
                  };


                  auto transform_sequential = [&]( auto instance_id)
                  {
                     auto& instance = state.instances.sequential[ instance_id];
                     return manager::admin::model::service::instance::Sequential{
                        instance.process,
                     };
                  };

                  common::algorithm::transform( service.instances.sequential(), result.instances.sequential, transform_sequential);
                  common::algorithm::transform( service.instances.concurrent(), result.instances.concurrent, transform_concurrent);

                  target.push_back( std::move( result));
               });
            }

            auto reservation( const manager::State& state)
            {
               return [ &state]( auto& instance)
               { 
                  manager::admin::model::Reservation result;
                  if( auto service_id = instance.reserved_service())
                     result.service = state.services[ service_id].information.name;

                  result.callee = instance.process;

                  if( auto& caller = instance.caller())
                  {
                     result.caller = caller.process;
                     result.correlation = caller.correlation;
                  }

                  return result;
               };
            }

            namespace transform::instance
            {
               auto sequential()
               {
                  return []( const manager::state::instance::Sequential& instance)
                  {
                     admin::model::instance::Sequential result;
                     result.process = instance.process;
                     result.alias = instance.alias;
                     
                     using State = decltype( result.state);
                     result.state = instance.state() == decltype( instance.state())::busy ? State::busy : State::idle;

                     common::log::line( verbose::log, "REMOVE - instance: ", instance);
                     common::log::line( verbose::log, "REMOVE - result: ", result);

                     return result;
                  };
               }

               auto concurrent()
               {
                  return []( const manager::state::instance::Concurrent& instance)
                  {
                     admin::model::instance::Concurrent result;
                     result.process = instance.process;
                     result.alias = instance.alias;
                     result.description = instance.description;
                     return result;
                  };
               }
               
            } // transform::instance


         } // <unnamed>
      } // local


      manager::admin::model::State state( const manager::State& state)
      {
         manager::admin::model::State result;

         auto transform_index = []( auto& index, auto& result, auto transformer)
         {
            index.for_each( [ &]( auto, auto, auto& instance){
               result.push_back( transformer( instance));
            });
         };

         transform_index( state.instances.sequential, result.instances.sequential, local::transform::instance::sequential());
         transform_index( state.instances.concurrent, result.instances.concurrent, local::transform::instance::concurrent());

         local::services( state, result.services);

         common::algorithm::transform( state.pending.lookups, result.pending, local::pending());

         common::algorithm::for_each( state.routes, [&result]( auto& pair)
         {
            manager::admin::model::Route route;
            route.target = pair.first;

            common::algorithm::transform( pair.second, result.routes, [&route]( auto& service)
            {
               route.service = service;
               return route;
            });
         });

         {
            auto transform_reservation = local::reservation( state);

            state.instances.sequential.for_each( [ transform_reservation, &result]( auto id, auto& ipc, auto& instance)
            {
               if( ! instance.idle())
                  result.reservations.push_back( transform_reservation( instance));
            });

         }



         return result;
      }


      configuration::model::service::Model configuration( const manager::State& state)
      {
         configuration::model::service::Model result;

         result.global.timeout = state.timeout;
         result.restriction = state.restriction;

         common::algorithm::sort( result.restriction.servers);

         result.services = common::algorithm::sort( common::algorithm::transform( state.routes, []( auto& pair)
         {
            configuration::model::service::Service result;
            result.name = pair.first;
            result.routes = pair.second;
            return result;
         }));

         state.services.for_each( [ &result]( auto service_id, auto& name, auto& service)
         {
            if( service.category == ".admin")
               return;

            if( ! service.timeout.duration)
               return;

            auto& configured = [&result, &service]() -> configuration::model::service::Service&
            {
               if( auto found = common::algorithm::find( result.services, service.information.name))
                  return *found;

               return result.services.emplace_back();
            }(); 
            
            configured.name = service.information.name;
            configured.timeout = service.timeout;
         });

         return result;
      }

   } // service::manager::transform
} // casual


