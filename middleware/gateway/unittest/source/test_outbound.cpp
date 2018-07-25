//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>
#include "common/unittest.h"

#include "common/mockup/process.h"

namespace casual
{
   using namespace common;

   namespace gateway
   {

      namespace local
      {
         namespace
         {
            struct Outbound
            {

               Outbound( const std::string& address)
                 : process{ "./bin/casual-gateway-outbound-tcp",
                     { "--address", address}}
               {

               }

               common::mockup::Process process;
            };

         }
      }


      TEST( casual_gateway_outbound_tcp, someTestCase)
      {

      }

   }

} // casual
