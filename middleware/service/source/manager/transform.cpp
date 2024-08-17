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

            auto service()
            {
               return []( auto& pair)
               {
                  auto& name = std::get< 0>( pair);
                  auto& value = std::get< 1>( pair);

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
                  result.execution.timeout = value.timeout;                        
                  result.metric.invoked = transform_metric( value.metric.invoked);
                  result.metric.pending = transform_metric( value.metric.pending);
                  result.metric.remote = value.metric.remote;
                  result.category = value.category;
                  result.transaction = value.transaction;
                  result.visibility = value.visibility.value_or( common::service::visibility::Type::discoverable);
                  result.metric.last = value.metric.last;

                  auto transform_concurrent = []( const auto& value)
                  {
                     return manager::admin::model::service::instance::Concurrent{
                        value.process(),
                        value.property.hops,
                        value.get().order
                     };
                  };


                  auto transform_sequential = [&]( const auto& value)
                  {
                     return manager::admin::model::service::instance::Sequential{
                        value.process(),
                     };
                  };

                  common::algorithm::transform( value.instances.sequential(), result.instances.sequential, transform_sequential);
                  common::algorithm::transform( value.instances.concurrent(), result.instances.concurrent, transform_concurrent);

                  return result;
               };
            }

            auto reservation()
            {
               return []( auto& pair)
               {
                  auto& instance = std::get< 1>( pair);

                  manager::admin::model::Reservation result;
                  result.service = instance.reserved_service().value_or( result.service);
                  result.callee = instance.process;

                  if( auto caller = instance.caller())
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

         common::algorithm::transform( state.instances.sequential, result.instances.sequential,
            common::predicate::composition( local::transform::instance::sequential(), common::predicate::adapter::second()));

         common::algorithm::transform( state.instances.concurrent, result.instances.concurrent,
            common::predicate::composition( local::transform::instance::concurrent(), common::predicate::adapter::second()));

         common::algorithm::transform( state.pending.lookups, result.pending, local::pending());

         common::algorithm::transform( state.services, result.services, local::service());

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

         common::algorithm::transform_if(
            state.instances.sequential, std::back_inserter( result.reservations), local::reservation(), []( auto& pair){ return ! pair.second.idle();}
         );

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

         common::algorithm::for_each( state.services, [ &result]( auto& pair)
         {
            auto& service = pair.second;
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


