//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "service/transform.h"
#include "service/common.h"

#include "common/environment.h"
#include "common/chronology.h"
#include "common/predicate.h"

namespace casual
{
   namespace service
   {
      namespace transform
      {

         namespace local
         {
            namespace
            {
               struct Instance
               {
                  auto operator() ( const manager::state::instance::Sequential& instance) const
                  {
                     manager::admin::model::instance::Sequential result;

                     result.process = instance.process;

                     auto transform_state = []( auto state)
                     {
                        using Enum = manager::state::instance::Sequential::State;
                        switch( state)
                        {
                           case Enum::idle: return manager::admin::model::instance::Sequential::State::idle;
                           case Enum::busy: return manager::admin::model::instance::Sequential::State::busy;
                        }
                        assert( ! "missmatch in enum transformation");
                     };

                     result.state = transform_state( instance.state());

                     return result;
                  }

                  auto operator() ( const manager::state::instance::Concurrent& instance) const
                  {
                     manager::admin::model::instance::Concurrent result;

                     result.process = instance.process;

                     return result;
                  }
               };

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

                     auto transform_mode = []( auto mode)
                     {
                        using Enum = decltype( mode);
                        switch( mode)
                        {
                           case Enum::automatic: return manager::admin::model::Service::Transaction::automatic;
                           case Enum::join: return manager::admin::model::Service::Transaction::join;
                           case Enum::atomic: return manager::admin::model::Service::Transaction::atomic;
                           case Enum::none: return manager::admin::model::Service::Transaction::none;
                           case Enum::branch: return manager::admin::model::Service::Transaction::branch;
                        }
                        assert( ! "unknown transaction mode");
                     }; 

                     manager::admin::model::Service result;

                     result.name = name;
                     result.execution.timeout = value.timeout;                        
                     result.metric.invoked = transform_metric( value.metric.invoked);
                     result.metric.pending = transform_metric( value.metric.pending);
                     result.metric.remote = value.metric.remote;
                     result.category = value.information.category;
                     result.transaction = transform_mode( value.information.transaction);
                     result.metric.last = value.metric.last;

                     auto transform_concurrent = []( const auto& value)
                     {
                        return manager::admin::model::service::instance::Concurrent{
                           value.process().pid,
                           value.property.hops
                        };
                     };


                     auto transform_sequential = [&]( const auto& value)
                     {
                        return manager::admin::model::service::instance::Sequential{
                           value.process().pid,
                        };
                     };

                     common::algorithm::transform( value.instances.sequential, result.instances.sequential, transform_sequential);
                     common::algorithm::transform( value.instances.concurrent, result.instances.concurrent, transform_concurrent);

                     return result;
                  };
               }


            } // <unnamed>
         } // local


         manager::admin::model::State state( const manager::State& state)
         {
            manager::admin::model::State result;

            common::algorithm::transform( state.instances.sequential, result.instances.sequential,
                  common::predicate::make_nested( local::Instance{}, common::extract::Second{}));

            common::algorithm::transform( state.instances.concurrent, result.instances.concurrent,
                  common::predicate::make_nested( local::Instance{}, common::extract::Second{}));

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

            return result;
         }


         configuration::model::service::Model configuration( const manager::State& state)
         {
            configuration::model::service::Model result;

            result.timeout = state.timeout;

            result.restrictions = common::algorithm::transform( state.restrictions, []( auto& pair)
            {
               configuration::model::service::Restriction result;
               result.alias = pair.first;
               result.services = pair.second;
               common::algorithm::sort( result.services);
               return result;
            });
            common::algorithm::sort( result.restrictions);

            result.services = common::algorithm::transform( state.routes, []( auto& pair)
            {
               configuration::model::service::Service result;
               result.name = pair.first;
               result.routes = pair.second;
               return result;
            });
            common::algorithm::sort( result.restrictions);

            common::algorithm::for_each( state.services, [&result]( auto& pair)
            {
               auto& service = pair.second;
               if( service.information.category == ".admin")
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

      } // transform
   } // service
} // casual


