//!
//! casual
//!


#include "broker/transform.h"

#include "common/environment.h"
#include "common/chronology.h"

namespace casual
{

   namespace broker
   {
      namespace transform
      {


         state::Service Service::operator () ( const common::message::Service& value) const
         {
            state::Service result;

            result.information.name = value.name;
            //result.information.timeout = value.timeout;
            result.information.transaction = value.transaction;
            result.information.category = value.category;

            // TODD: set against configuration


            return result;
         }


         common::process::Handle Instance::operator () ( const state::instance::Local& value) const
         {
            return value.process;
         }


         namespace local
         {
            namespace
            {
               struct Instance
               {
                  admin::instance::LocalVO operator() ( const state::instance::Local& instance) const
                  {
                     admin::instance::LocalVO result;

                     result.process = instance.process;
                     result.invoked = instance.invoked;
                     result.last = instance.last();
                     result.state = static_cast< admin::instance::LocalVO::State>( instance.state());

                     return result;
                  }

                  admin::instance::RemoteVO operator() ( const state::instance::Remote& instance) const
                  {
                     admin::instance::RemoteVO result;

                     result.process = instance.process;
                     result.invoked = instance.invoked;
                     result.last = instance.last();

                     return result;
                  }



               };

               struct Pending
               {
                  admin::PendingVO operator() ( const state::service::Pending& value) const
                  {
                     admin::PendingVO result;

                     result.process = value.request.process;
                     result.requested = value.request.requested;

                     return result;
                  }
               };

               struct Service
               {
                  admin::ServiceVO operator() ( const state::Service& value) const
                  {
                     auto transform_metric = []( const state::service::Metric& value)
                           {
                              admin::service::Metric result;
                              result.invoked = value.invoked();
                              result.last = value.used();
                              result.total = value.total();
                              return result;
                           };

                     admin::ServiceVO result;

                     result.name = value.information.name;
                     result.timeout = value.information.timeout;
                     result.metrics = transform_metric( value.metric);
                     result.pending.count = value.pending.count();
                     result.pending.total = value.pending.total();
                     result.category = value.information.category;
                     result.transaction = common::cast::underlying( value.information.transaction);



                     auto transform_remote = []( const state::service::instance::Remote& value)
                           {
                              return admin::service::instance::Remote{
                                 value.process().pid,
                                 value.invoked,
                                 value.hops(),
                                 value.get().last()
                              };
                           };


                     auto transform_local = [&]( const state::service::instance::Local& value)
                           {
                              return admin::service::instance::Local{
                                 value.process().pid,
                                 transform_metric( value.metric)
                              };
                           };

                     common::range::transform( value.instances.local, result.instances.local, transform_local);
                     common::range::transform( value.instances.remote, result.instances.remote, transform_remote);

                     return result;
                  }
               };

            } // <unnamed>
         } // local

         admin::StateVO state( const broker::State& state)
         {
            admin::StateVO result;

            common::range::transform( state.instances.local, result.instances.local,
                  common::chain::Nested::link( local::Instance{}, common::extract::Second{}));

            common::range::transform( state.instances.remote, result.instances.remote,
                  common::chain::Nested::link( local::Instance{}, common::extract::Second{}));

            common::range::transform( state.pending.requests, result.pending, local::Pending{});

            common::range::transform( state.services, result.services,
                  common::chain::Nested::link( local::Service{}, common::extract::Second{}));


            return result;
         }


      } // transform
   } // broker

} // casual


