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

         namespace configuration
         {


            state::Service Service::operator () ( const config::domain::Service& service) const
            {
               state::Service result;

               result.information.name = service.name;
               result.information.timeout = common::chronology::from::string( service.timeout);

               return result;
            }

         } // configuration



         state::Service Service::operator () ( const common::message::Service& value) const
         {
            state::Service result;

            result.information.name = value.name;
            //result.information.timeout = value.timeout;
            result.information.transaction = value.transaction;
            result.information.type = value.type;

            // TODD: set against configuration


            return result;
         }


         common::process::Handle Instance::operator () ( const state::Instance& value) const
         {
            return value.process;
         }


         namespace local
         {
            namespace
            {
               struct Instance
               {
                  admin::InstanceVO operator() ( const state::Instance& instance) const
                  {
                     admin::InstanceVO result;

                     result.process = instance.process;
                     result.invoked = instance.invoked;
                     result.last = instance.last();
                     result.state = static_cast< admin::InstanceVO::State>( instance.state());

                     return result;
                  }



               };

               struct Pending
               {
                  admin::PendingVO operator() ( const common::message::service::lookup::Request& value) const
                  {
                     admin::PendingVO result;

                     result.process = value.process;
                     result.requested = value.requested;

                     return result;
                  }
               };

               struct Service
               {
                  admin::ServiceVO operator() ( const state::Service& value) const
                  {
                     admin::ServiceVO result;

                     result.name = value.information.name;
                     result.timeout = value.information.timeout;
                     result.lookedup = value.lookedup;
                     result.type = value.information.type;
                     result.transaction = common::cast::underlying( value.information.transaction);

                     auto transform_instance = []( const state::service::Instance& value)
                           {
                              return admin::ServiceVO::Instance{
                                 value.process().pid,
                                 value.hops()
                              };
                           };

                     common::range::transform( value.instances.local, result.instances, transform_instance);
                     common::range::transform( value.instances.remote, result.instances, transform_instance);

                     return result;
                  }
               };

            } // <unnamed>
         } // local

         admin::StateVO state( const broker::State& state)
         {
            admin::StateVO result;

            common::range::transform( state.instances, result.instances,
                  common::chain::Nested::link( local::Instance{}, common::extract::Second{}));

            common::range::transform( state.pending.requests, result.pending, local::Pending{});

            common::range::transform( state.services, result.services,
                  common::chain::Nested::link( local::Service{}, common::extract::Second{}));


            return result;
         }


      } // transform
   } // broker

} // casual


