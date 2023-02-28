//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/unittest/utility.h"

#include "queue/common/log.h"
#include "queue/common/queue.h"
#include "queue/manager/admin/services.h"

#include "serviceframework/service/protocol/call.h"

#include "common/unittest.h"

namespace casual
{
   namespace queue::unittest
   {
      manager::admin::model::State state()
      {
         common::unittest::service::wait::until::advertised( queue::manager::admin::service::name::state);
         return serviceframework::service::protocol::binary::Call{}( queue::manager::admin::service::name::state).extract< manager::admin::model::State>();
      }


      std::vector< manager::admin::model::Message> messages( const std::string& queue)
      {
         using Call = serviceframework::service::protocol::binary::Call;
         return Call{}( manager::admin::service::name::messages::list, Call::Flags{}, queue).extract< std::vector< manager::admin::model::Message>>();
      }

      namespace scale
      {
         void aliases( const std::vector< manager::admin::model::scale::Alias>& aliases)
         {
            using Call = serviceframework::service::protocol::binary::Call;
            Call{}( manager::admin::service::name::forward::scale::aliases, Call::Flags{}, aliases);
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
            common::log::line( verbose::log, "name: ", name);

            // wait until it's known...
            auto reply = queue::Lookup{ name, queue::Lookup::Semantic::wait}();
            common::log::line( verbose::log, "reply: ", reply);
         }
      } // wait::until

   } // domain::unittest
} // casual