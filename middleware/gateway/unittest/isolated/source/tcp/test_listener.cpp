//!
//! casual 
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "gateway/manager/listener.h"

#include "common/communication/ipc.h"

namespace casual
{
   namespace gateway
   {
      namespace local
      {
         namespace
         {
            struct Listener : manager::Listener
            {
               template< typename T>
               Listener( T&& value) : manager::Listener{ std::forward< T>( value)}
               {
                  start();
                  common::communication::ipc::Helper ipc;

                  message::manager::listener::Event message;
                  ipc.blocking_receive( message, correlation());
                  event( message);
               }

               ~Listener()
               {
                  if( running())
                  {
                     common::signal::thread::scope::Block block;
                     shutdown();

                     common::communication::ipc::Helper ipc;

                     message::manager::listener::Event message;
                     ipc.blocking_receive( message, correlation());
                     event( message);
                  }
               }
            };

         } // <unnamed>
      } // local

      TEST( casual_gateway_tcp_listener, instansiate_localhost_port_6666__expect_running_state)
      {
         common::unittest::Trace trace;
         local::Listener listener{ common::communication::tcp::Address{ ":6666"}};
         EXPECT_TRUE( listener.state() == manager::Listener::State::running) << "listener: " << listener;

      }

      TEST( casual_gateway_tcp_listener, instansiate_2__ip_127_0_0_1_port_6666__expect_error_state_on_second)
      {
         common::unittest::Trace trace;

         local::Listener listener1{ common::communication::tcp::Address{ "127.0.0.1:6666"}};
         EXPECT_TRUE( listener1.state() == manager::Listener::State::running) << "listener: " << listener1;

         local::Listener listener2{ common::communication::tcp::Address{ "127.0.0.1:6666"}};
         EXPECT_TRUE( listener2.state() == manager::Listener::State::error) << "listener: " << listener2;

      }

   } // gateway


} // casual
