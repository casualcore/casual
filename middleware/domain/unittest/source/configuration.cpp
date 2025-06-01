//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/unittest/configuration.h"
#include "domain/common.h"
#include "domain/manager/admin/server.h"
#include "domain/manager/admin/call.h"


#include "common/event/listen.h"

#include "common/message/dispatch/handle.h"
#include "common/message/dispatch.h"


namespace casual
{
   using namespace common;

   namespace domain::unittest::configuration
   {
      namespace local
      {
         namespace
         {
            auto call( casual::configuration::user::Model wanted, std::string_view service)
            {
               std::vector< common::strong::correlation::id> tasks;

               auto condition = common::event::condition::compose(
                  common::event::condition::prelude( [&]()
                  {
                     tasks = manager::admin::call::service< std::vector< common::strong::correlation::id>>( service, wanted);
                  }),
                  common::event::condition::done( [&tasks](){ return tasks.empty();})
               );

               // listen for events
               common::event::listen( 
                  condition,
                  message::dispatch::handler( communication::ipc::inbound::device(),
                     [ &tasks]( message::event::Task& event)
                     {
                        log::debug( "event: ", event);

                        if( event.done())
                           if( algorithm::find( tasks, event.correlation))
                              tasks.clear();
                     })
                  );

               return configuration::get();
            }
         } // <unnamed>
      } // local

      casual::configuration::user::Model get()
      {
         Trace trace{ "domain::unittest::configuration::get"};

         return manager::admin::call::service< casual::configuration::user::Model>( manager::admin::service::name::configuration::get);
      }

      casual::configuration::user::Model post( casual::configuration::user::Model wanted)
      {
         Trace trace{ "domain::unittest::configuration::post"};

         return local::call( std::move( wanted), manager::admin::service::name::configuration::post);
      }

      casual::configuration::user::Model put( casual::configuration::user::Model wanted)
      {
         Trace trace{ "domain::unittest::configuration::put"};

         return local::call( std::move( wanted), manager::admin::service::name::configuration::put);
      }

   } // domain::unittest::configuration
} // casual