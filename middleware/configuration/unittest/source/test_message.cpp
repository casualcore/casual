//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>
#include "common/unittest.h"

#include "common/message/domain.h"
#include "common/communication/ipc.h"

#include "common/unittest/eventually/send.h"


namespace casual
{

   namespace configuration
   {
      using namespace common::message::domain::configuration;

      TEST( configuration_message, reply_default_marshal)
      {
         common::unittest::Trace trace;

         {
            configuration::Reply reply;
            common::unittest::eventually::send( common::communication::ipc::inbound::ipc(), reply);
         }

         {
            configuration::Reply reply;
            common::communication::ipc::blocking::receive( common::communication::ipc::inbound::device(), reply);

            EXPECT_TRUE( reply.domain.name.empty());
         }
      }


      TEST( configuration_message, reply_marshal)
      {
         common::unittest::Trace trace;

         {
            configuration::Reply reply;
            reply.domain.name = "test-domain";
            common::unittest::eventually::send( common::communication::ipc::inbound::ipc(), reply);
         }

         {
            configuration::Reply reply;
            common::communication::ipc::blocking::receive( common::communication::ipc::inbound::device(), reply);

            EXPECT_TRUE( reply.domain.name == "test-domain") << " reply.domain.name: " <<  reply.domain.name;
         }
      }

   } // configuration


} // casual
