//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/unittest/utility.h"

#include "queue/common/log.h"
#include "queue/common/queue.h"
#include "queue/manager/admin/services.h"

#include "service/protocol/call.h"
#include "service/unittest/utility.h"

#include "common/unittest.h"
#include "common/communication/instance.h"

namespace casual
{
   namespace queue::unittest
   {
      manager::admin::model::State state()
      {
         service::unittest::wait::until::advertised( queue::manager::admin::service::name::state);
         return casual::service::protocol::binary::Call{}( queue::manager::admin::service::name::state).extract< manager::admin::model::State>();
      }


      namespace advertise
      {
         void remote( std::vector< std::string> queues, const common::process::Handle& process)
         {
            auto transform_queue = []( auto& name )
            {
               queue::ipc::message::advertise::Queue result;
               result.name = name;
               return result;
            };

            ipc::message::Advertise message;
            message.process = process;
            message.order = 1;
            message.queues.add = common::algorithm::transform( queues, transform_queue);
            message.directive = decltype( message.directive)::update;
            message.alias = "foo";


            common::communication::device::blocking::send( 
               common::communication::instance::outbound::queue::manager::device(), message);  
         }

         void remote( std::vector< std::string> queues)
         {
            remote( std::move( queues), common::process::handle());
         }

      } // advertise

      std::vector< manager::admin::model::Message> messages( const std::string& queue)
      {
         using Call = casual::service::protocol::binary::Call;
         return Call{}( manager::admin::service::name::messages::list, queue).extract< std::vector< manager::admin::model::Message>>();
      }

      namespace scale
      {
         void aliases( const std::vector< manager::admin::model::scale::Alias>& aliases)
         {
            using Call = casual::service::protocol::binary::Call;
            Call{}( manager::admin::service::name::forward::scale::aliases, aliases);
         }

         namespace all::forward
         {
            void aliases( platform::size::type instances)
            {
               auto state = unittest::state();

               auto scale_forward = [instances]( auto& forward)
               {
                  manager::admin::model::scale::Alias result;
                  result.name = forward.alias;
                  result.instances = instances;
                  return result;
               };

               auto aliases = common::algorithm::transform( state.forward.services, scale_forward);
               common::algorithm::transform( state.forward.queues, aliases, scale_forward);

               unittest::scale::aliases( aliases);
            }
         } // all::forward

      } // scale

      namespace wait::until
      {
         void advertised( std::string_view name)
         {
            Trace trace{ "domain::unittest::wait::until::advertised"};
            common::log::debug( "name: ", name);

            // wait until it's known...
            auto reply = queue::Lookup{ name, queue::Lookup::Action::any, queue::Lookup::Semantic::wait}();
            common::log::debug( "reply: ", reply);
         }
      } // wait::until

   } // domain::unittest
} // casual
