//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "server/start.h"
#include "server/internal/start.h"

#include "server/service.h"
#include "server/argument.h"
#include "server/context.h"


#include "common/message/dispatch/handle.h"
#include "common/communication/instance.h"

namespace casual
{
   namespace server
   {
      inline namespace v1
      {
         namespace local
         {
            namespace
            {
               namespace transform
               {
                  
                  server::Arguments arguments(
                        std::vector< argument::Service> services,
                        std::vector< argument::transaction::Resource> resources)
                  {
                     server::Arguments result;

                     auto transform_server = []( argument::Service& service)
                     {
                        return server::Service{
                           .name = std::move( service.name),
                           .function = std::move( service.function),
                           .transaction = service.transaction,
                           .visibility = service.visibility,
                           .category = std::move( service.category),
                        };

                     };

                     common::algorithm::transform( services, result.services, transform_server);
                     result.resources = std::move( resources);

                     return result;
                  }
               } // transform

            } // <unnamed>
         } // local

         void start( std::vector< argument::Service> services, std::vector< argument::transaction::Resource> resources)
         {
            server::internal::start( local::transform::arguments( services, resources), nullptr);
         }

         void start( std::vector< argument::Service> services)
         {
            start( std::move( services), {});
         }

      } // v1

      } // server
} // casual
