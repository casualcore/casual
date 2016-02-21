//!
//! casual 
//!

#include <gtest/gtest.h>


#include "common/mockup/process.h"
#include "common/mockup/domain.h"

#include "common/trace.h"


namespace casual
{
   using namespace common;

   namespace gateway
   {

      namespace local
      {
         namespace
         {
            struct Inbound
            {

               Inbound( platform::queue_id_type ipc)
                 : process{ "./bin/casual-gateway-inbound-ipc", {
                        "-ipc", std::to_string( ipc)}}
               {

               }

               common::mockup::Process process;
            };


            struct Domain
            {
               //
               // inbound ipc act as the remote domains outbound gateway
               //
               Domain()
                  : inbound{ communication::ipc::inbound::id()}
               {
                  //
                  // Do the connection dance...
                  //


               }

               common::mockup::domain::Broker broker;
               common::mockup::domain::transaction::Manager tm;


               Inbound inbound;
            };

         } // <unnamed>
      } // local

      TEST( casual_gateway_inbound, shutdown_before_connection__expect_gracefull_shutdown)
      {
         Trace trace{ "casual_gateway_inbound shutdown_before_connection__expect_gracefull_shutdown"};

         EXPECT_NO_THROW({
            local::Inbound inbound{ communication::ipc::inbound::id()};
         });

      }

   } // gateway


} // casual
