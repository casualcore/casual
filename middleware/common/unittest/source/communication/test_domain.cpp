//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "common/communication/domain.h"

namespace casual
{
   namespace common
   {
      namespace communication
      {
         TEST( common_communication_domain, socket_create)
         {
            EXPECT_NO_THROW({
               auto socket = domain::native::create();
            });
            
         }

         TEST( common_communication_domain, address_create)
         {
            domain::Address addres;
            
            EXPECT_TRUE( ! addres.name().empty());
         }  

         TEST( common_communication_domain, socket_bind)
         {
            domain::Address addres;
            auto socket = domain::native::create();

            domain::native::bind( socket, addres);
         }   
         
      } // communication
      
   } // common
} // casual

