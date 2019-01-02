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


         manager::state::Service Service::operator () ( const common::message::Service& value) const
         {
            manager::state::Service result;

            result.information.name = value.name;
            //result.information.timeout = value.timeout;
            result.information.transaction = value.transaction;
            result.information.category = value.category;

            // TODD: set against configuration


            return result;
         }


         common::process::Handle Instance::operator () ( const manager::state::instance::Sequential& value) const
         {
            return value.process;
         }

         namespace local
         {
            namespace
            {
               struct Instance
               {
                  manager::admin::instance::SequentialVO operator() ( const manager::state::instance::Sequential& instance) const
                  {
                     manager::admin::instance::SequentialVO result;

                     result.process = instance.process;
                     result.state = static_cast< manager::admin::instance::SequentialVO::State>( instance.state());

                     return result;
                  }

                  manager::admin::instance::ConcurrentVO operator() ( const manager::state::instance::Concurrent& instance) const
                  {
                     manager::admin::instance::ConcurrentVO result;

                     result.process = instance.process;

                     return result;
                  }
               };

               struct Pending
               {
                  manager::admin::PendingVO operator() ( const manager::state::service::Pending& value) const
                  {
                     manager::admin::PendingVO result;

                     result.process = value.request.process;
                     result.requested = value.request.requested;

                     return result;
                  }
               };

               struct Service
               {
                  manager::admin::ServiceVO operator() ( const manager::state::Service& value) const
                  {
                     auto transform_metric = []( const auto& value)
                     {
                        manager::admin::service::Metric result;
                        result.count = value.count;
                        result.total = value.total;
                        result.limit.min = value.limit.min;
                        result.limit.max = value.limit.max;
                        return result;
                     };

                     manager::admin::ServiceVO result;

                     result.name = value.information.name;
                     result.timeout = value.information.timeout;
                     result.metrics = transform_metric( value.metric);
                     result.pending = transform_metric( value.pending);
                     result.remote_invocations = value.remote_invocations();
                     result.category = value.information.category;
                     result.transaction = common::cast::underlying( value.information.transaction);
                     result.last = value.last();

                     auto transform_remote = []( const auto& value)
                           {
                              return manager::admin::service::instance::Remote{
                                 value.process().pid,
                                 value.hops(),
                              };
                           };


                     auto transform_local = [&]( const auto& value)
                           {
                              return manager::admin::service::instance::Local{
                                 value.process().pid,
                              };
                           };

                     common::algorithm::transform( value.instances.sequential, result.instances.sequential, transform_local);
                     common::algorithm::transform( value.instances.concurrent, result.instances.concurrent, transform_remote);

                     return result;
                  }
               };
            } // <unnamed>
         } // local


         manager::admin::StateVO state( const manager::State& state)
         {
            manager::admin::StateVO result;

            common::algorithm::transform( state.instances.sequential, result.instances.sequential,
                  common::predicate::make_nested( local::Instance{}, common::extract::Second{}));

            common::algorithm::transform( state.instances.concurrent, result.instances.concurrent,
                  common::predicate::make_nested( local::Instance{}, common::extract::Second{}));

            common::algorithm::transform( state.pending.requests, result.pending, local::Pending{});

            common::algorithm::transform( state.services, result.services,
                  common::predicate::make_nested( local::Service{}, common::extract::Second{}));

            return result;
         }

      } // transform
   } // service
} // casual


