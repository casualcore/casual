//!
//! casual 
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "gateway/manager/listener.h"

namespace casual
{
   namespace gateway
   {
      TEST( casual_gateway_tcp_listener, instansiate_localhost_port_6666__expect_running_state)
      {
         CASUAL_UNITTEST_TRACE();

         manager::Listener listener{ common::communication::tcp::Address{ ":6666"}};
         EXPECT_TRUE( listener.state() == manager::Listener::State::running) << "listener: " << listener;

      }

      TEST( casual_gateway_tcp_listener, instansiate_2__ip_127_0_0_1_port_6666__expect_error_state_on_second)
      {
         CASUAL_UNITTEST_TRACE();

         manager::Listener listener1{ common::communication::tcp::Address{ "127.0.0.1:6666"}};
         EXPECT_TRUE( listener1.state() == manager::Listener::State::running) << "listener: " << listener1;

         manager::Listener listener2{ common::communication::tcp::Address{ "127.0.0.1:6666"}};
         EXPECT_TRUE( listener2.state() == manager::Listener::State::error) << "listener: " << listener2;

      }

   } // gateway


} // casual
