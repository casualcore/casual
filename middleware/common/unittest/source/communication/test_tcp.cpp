//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/unittest/thread.h"

#include "common/communication/tcp.h"
#include "common/exception/handle.h"

#include "common/message/service.h"


namespace casual
{
   namespace common::communication
   {
      namespace local
      {
         namespace
         {
            void simple_server( tcp::Address address)
            {
               try
               {
                  tcp::Listener listener{ std::move( address)};

                  std::vector< Socket> connections;

                  while( true)
                  {
                     connections.push_back( listener());

                     log::line( log::debug, "connections: ", connections);
                  }
               }
               catch( ...)
               {
                  exception::sink::log();
               }
            }

            template< typename T>
            bool boolean( T&& value)
            {
               return static_cast< bool>( value);
            }


            tcp::Address address()
            {
               static long port = 23666;
               return { string::compose(  "127.0.0.1:", ++port)};
            }

         } // <unnamed>
      } // local

      TEST( common_communication_tcp, address_host_port)
      {   
         common::unittest::Trace trace;

         {
            tcp::Address address{ "127.0.0.1:666"};
            EXPECT_TRUE( address.host() == "127.0.0.1") << "host: " << address.host();
            EXPECT_TRUE( address.port() == "666") << "port: " << address.host();
         }

         {
            tcp::Address address{ "127.0.0.1"};
            EXPECT_TRUE( address.host() == "127.0.0.1") << "host: " << address.host();
            EXPECT_TRUE( address.port().empty()) << "port: " << address.host();
         }
      }

      TEST( common_communication_tcp, connect_to_non_existent_port__expect_connection_refused)
      {   
         common::unittest::Trace trace;

         EXPECT_CODE( {
            tcp::connect( local::address());
         }, code::casual::communication_refused);
      }

      TEST( common_communication_tcp, listener_port)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            tcp::Listener listener{ local::address()};
         });
      }


      TEST( common_communication_tcp, listener_port__connect_to_port__expect_connection)
      {
         common::unittest::Trace trace;

         const auto address = local::address();

         unittest::Thread server{ &local::simple_server, address};

         auto socket = tcp::retry::connect( address, { { std::chrono::milliseconds{ 1}, 0}});

         EXPECT_TRUE( local::boolean( socket));
      }


      TEST( common_communication_tcp, listener_port__connect_to_port_10_times__expect_connections)
      {
         common::unittest::Trace trace;

         const auto address = local::address();

         unittest::Thread server{ &local::simple_server, address};

         std::vector< Socket> connections;

         algorithm::for_n< 10>( [&](){
            connections.push_back( tcp::retry::connect( address, { { std::chrono::milliseconds{ 1}, 0}}));
         });

         for( auto& socket : connections)
            EXPECT_TRUE( local::boolean( socket)) << CASUAL_NAMED_VALUE( socket);
      }

      namespace local
      {
         namespace
         {
            namespace echo
            {
               void worker( Socket&& socket)
               {
                  try
                  {

                     while( true)
                     {
                        tcp::native::send(
                           socket,
                           tcp::native::receive( socket, {}),
                           {});
                     }
                  }
                  catch( ...)
                  {
                     exception::handle( log::debug);
                  }

               }

               void server( tcp::Address address)
               {
                  std::vector< unittest::Thread> workers;

                  try
                  {
                     tcp::Listener listener{ std::move( address)};

                     while( true)
                     {
                        workers.emplace_back( &echo::worker, listener());
                     }
                  }
                  catch( ...)
                  {
                     exception::handle( log::debug);
                  }
               }

            } // echo
         } // <unnamed>
      } // local


      TEST( common_communication_tcp, echo_server_port__connect_to_port__expect_connection)
      {
         common::unittest::Trace trace;

         const auto address = local::address();

         unittest::Thread server{ &local::echo::server, address};

         auto socket = tcp::retry::connect( address, { { std::chrono::milliseconds{ 1}, 0}});

         EXPECT_TRUE( local::boolean( socket));

         platform::binary::type payload{ 1,2,3,4,5,6,7,8,9};
         auto correlation = uuid::make();

         // send
         {
            tcp::message::Complete message;
            message.type = common::message::Type::process_lookup_request;
            message.correlation = correlation;
            message.payload = payload;

            EXPECT_TRUE( tcp::native::send( socket, message, {}) == correlation);
         }

         // receive
         {
            auto message = tcp::native::receive( socket, {});

            EXPECT_TRUE( message.correlation == correlation);
            EXPECT_TRUE( message.type == common::message::Type::process_lookup_request);
            EXPECT_TRUE( message.payload == payload) << "message: " << message;
         }
      }

      TEST( common_communication_tcp, echo_server_port__10_connect_to_port__expect_echo_from_10)
      {
         common::unittest::Trace trace;

         const auto address = local::address();

         unittest::Thread server{ &local::echo::server, address};

         std::vector< Socket> connections( 10);

         for( auto& socket : connections)
         {
            socket = tcp::retry::connect( address, { { std::chrono::milliseconds{ 1}, 0}});
            EXPECT_TRUE( local::boolean( socket));
         }


         platform::binary::type payload{ 1,2,3,4,5,6,7,8,9};
         auto correlation = uuid::make();

         for( auto& socket : connections)
         {
            // send
            tcp::message::Complete message;
            message.type = common::message::Type::process_lookup_request;
            message.correlation = correlation;
            message.payload = payload;

            EXPECT_TRUE( tcp::native::send( socket, message, {}) == correlation);
         }

         // receive
         for( auto& socket : connections)
         {
            auto message = tcp::native::receive( socket, {});

            EXPECT_TRUE( message.correlation == correlation);
            EXPECT_TRUE( message.type == common::message::Type::process_lookup_request);
            EXPECT_TRUE( message.payload == payload) << "message: " << message;
         }
      }

      TEST( common_communication_tcp, echo_server_port__tcp_device_send_receive__expect_connection)
      {
         common::unittest::Trace trace;

         const auto address = local::address();

         unittest::Thread server{ &local::echo::server, address};

         tcp::Duplex tcp{ tcp::retry::connect( address, { { std::chrono::milliseconds{ 1}, 0}})};


         auto send = [&](){
            common::message::service::lookup::Request message;
            message.process = process::handle();
            message.requested = "testservice";

            return device::blocking::send( tcp, message);
         };


         auto correlation = send();


         // receive (the echo)
         {
            common::message::service::lookup::Request message;

            device::blocking::receive( tcp, message, correlation);

            EXPECT_TRUE( message.process == process::handle());
            EXPECT_TRUE( message.requested == "testservice");
         }
      }


      TEST( common_communication_tcp, echo_server_port__tcp_device_send_receive__10k_payload)
      {
         common::unittest::Trace trace;

         const auto address = local::address();

         unittest::Thread server{ &local::echo::server, address};

         tcp::Duplex tcp{ tcp::retry::connect( address, { { std::chrono::milliseconds{ 1}, 0}})};

         auto send_message = unittest::random::message( 10 * 1024);

         auto correlation = device::blocking::send( tcp, send_message);


         // receive (the echo)
         {
            unittest::Message receive_message;
            device::blocking::receive( tcp, receive_message, correlation);

            EXPECT_TRUE( common::algorithm::equal( receive_message.payload, send_message.payload));
         }
      }

      TEST( common_communication_tcp, echo_server_port__tcp_device_send_receive_100k_payload)
      {
         common::unittest::Trace trace;

         const auto address = local::address();

         unittest::Thread server{ &local::echo::server, address};

         tcp::Duplex tcp{ tcp::retry::connect( address, { { std::chrono::milliseconds{ 1}, 0}})};

         auto send_message = unittest::random::message( 100 * 1024);

         auto correlation = device::blocking::send( tcp, send_message);


         // receive (the echo)
         {
            unittest::Message receive_message;
            device::blocking::receive( tcp, receive_message, correlation);

            EXPECT_TRUE( common::algorithm::equal( receive_message.payload, send_message.payload));
         }
      }

      TEST( common_communication_tcp, echo_server_port__tcp_device_send_receive__1M_paylad)
      {
         common::unittest::Trace trace;

         const auto address = local::address();

         unittest::Thread server{ &local::echo::server, address};

         tcp::Duplex tcp{ tcp::retry::connect( address, { { std::chrono::milliseconds{ 1}, 0}})};

         auto send_message = unittest::random::message( 1024 * 1024);;

         auto correlation = device::blocking::send( tcp, send_message);


         // receive (the echo)
         {
            unittest::Message receive_message;
            device::blocking::receive( tcp, receive_message, correlation);

            EXPECT_TRUE( common::algorithm::equal( receive_message.payload, send_message.payload));
         }
      }


      TEST( common_communication_tcp, echo_server_port__tcp_device_send_receive__10M_payload)
      {
         common::unittest::Trace trace;

         const auto address = local::address();

         unittest::Thread server{ &local::echo::server, address};

         tcp::Duplex tcp{ tcp::retry::connect( address, { { std::chrono::milliseconds{ 1}, 0}})};

         auto send_message = unittest::random::message( 10 * 1024 * 1024);
         unittest::random::range( send_message.payload);

         auto correlation = device::blocking::send( tcp, send_message);


         // receive (the echo)
         {
            unittest::Message receive_message;
            device::blocking::receive( tcp, receive_message, correlation);

            EXPECT_TRUE( common::algorithm::equal( receive_message.payload, send_message.payload));
         }
      }
   } // common::communication
} // casual
