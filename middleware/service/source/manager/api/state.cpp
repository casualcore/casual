//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/service/manager/api/state.h"

#include "service/manager/admin/api.h"

#include "common/algorithm.h"

namespace casual
{
   using namespace common;
   namespace service::manager::api
   {

      inline namespace v1 
      {
         namespace local
         {
            namespace
            {
               namespace transform
               {
                  auto state( admin::model::State state)
                  {
                     Model result;

                     auto transform_service = []( auto& service)
                     {
                        model::Service result;
                        result.name = service.name;
                        result.category = service.category;
                        if( service.execution.timeout.duration)
                           result.timeout = service.execution.timeout.duration.value();

                        auto tranform_mode = []( auto mode)
                        {
                           using Enum = decltype( mode);
                           switch( mode)
                           {
                              case Enum::automatic: return model::Service::Transaction::automatic;
                              case Enum::join: return model::Service::Transaction::join;
                              case Enum::atomic: return model::Service::Transaction::atomic;
                              case Enum::none: return model::Service::Transaction::none;
                              case Enum::branch: return model::Service::Transaction::branch;
                           }
                           assert( ! "unknown transaction mode");
                        };

                        result.transaction = tranform_mode( service.transaction);

                        auto transform_metric = []( auto& metric)
                        {
                           model::service::Metric result;

                           auto transform_sub = []( auto metric)
                           {
                              model::Metric result;
                              result.count = metric.count;
                              result.total = metric.total;
                              result.limit.min = metric.limit.min;
                              result.limit.max = metric.limit.max;

                              return result;
                           };
                           result.invoked = transform_sub( metric.invoked);
                           result.pending = transform_sub( metric.pending);

                           result.last = metric.last;
                           result.remote = metric.remote;

                           return result;
                        };

                        result.metric = transform_metric( service.metric);

                        auto tranform_sequential = []( auto& instance)
                        {
                           model::service::instance::Sequential result;
                           result.pid = instance.pid.value();
                           return result;
                        };
                        algorithm::transform( service.instances.sequential, result.instances.sequential, tranform_sequential);

                        auto tranform_concurrent = []( auto& instance)
                        {
                           model::service::instance::Concurrent result;
                           result.pid = instance.pid.value();
                           result.hops = instance.hops;
                           return result;
                        };
                        algorithm::transform( service.instances.concurrent, result.instances.concurrent, tranform_concurrent);

                        return result;
                     };

                     algorithm::transform( state.services, result.services, transform_service);

                     return result;
                  }

               } // transform
               
            } // <unnamed>
         } // local

         Model state()
         {
            return local::transform::state( admin::api::state());
         }

      } // v1

   } // service::manager::api
} // casual