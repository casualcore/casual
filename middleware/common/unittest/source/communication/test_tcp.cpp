//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "common/communication/tcp.h"
#include "common/exception/capture.h"

#include "common/message/service.h"


namespace casual
{
   namespace common::communication
   {
      namespace local
      {
         namespace
         {
            tcp::Address address()
            {
               static long port = 7010;
               return { string::compose(  "127.0.0.1:", ++port)};
            }

            Socket connect( tcp::Address address)
            {
               return unittest::eventually::succeed( [ address = std::move( address)]()
               {
                  return tcp::connect( address);
               });
            }

            auto spawn_tcp_server( const tcp::Address& address)
            {
               auto path = "${CMAKE_BINARY_DIR}/middleware/common/bin/unittest_tcp_server";

               auto pid = common::process::spawn( path, { "--listen", address});

               return execute::scope( [pid]()
               {
                  signal::send( pid, code::signal::terminate);
                  process::wait( pid);
               });
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
         auto server_scope = local::spawn_tcp_server( address);

         auto socket = local::connect( address);

         EXPECT_TRUE( socket);
      }


      TEST( common_communication_tcp, listener_port__connect_to_port_10_times__expect_connections)
      {
         common::unittest::Trace trace;

         const auto address = local::address();
         auto server_scope = local::spawn_tcp_server( address);

         std::vector< Socket> connections;

         algorithm::for_n< 10>( [&](){
            connections.push_back( local::connect( address));
         });

         for( auto& socket : connections)
            EXPECT_TRUE( socket) << trace.compose( "socket: ", socket);
      }

      TEST( common_communication_tcp, echo_server_port__connect_to_port__expect_connection)
      {
         common::unittest::Trace trace;

         auto const address = local::address();
         auto server_scope = local::spawn_tcp_server( address);
      
         auto socket = local::connect( address);
         EXPECT_TRUE( socket);

         tcp::Duplex tcp{ std::move( socket)};

         auto payload = unittest::random::binary( 1024);
         auto correlation = strong::correlation::id{ uuid::make()};

         // send
         {
            tcp::message::Complete complete{ 
               common::message::Type::process_lookup_request, 
               correlation, 
               payload};

            EXPECT_TRUE( device::blocking::send( tcp, std::move( complete)));
         }

         // receive
         {
            auto complete = device::blocking::next( tcp);

            EXPECT_TRUE( complete);
            EXPECT_TRUE( complete.correlation() == correlation);
            EXPECT_TRUE( complete.type() == common::message::Type::process_lookup_request);
            EXPECT_TRUE( complete.payload == payload) << "complete: " << complete;
         }

      }

      TEST( common_communication_tcp, echo_server_port__10_connect_to_port__expect_echo_from_10)
      {
         common::unittest::Trace trace;

         auto const address = local::address();
         auto server_scope = local::spawn_tcp_server( address);

         auto connections = algorithm::generate_n< 10>( [&address]()
         {
            auto socket = local::connect( address);
            EXPECT_TRUE( socket);
            return tcp::Duplex{ std::move( socket)};
         });


         auto payload = unittest::random::binary( 1024);
         auto correlation = strong::correlation::id{ uuid::make()};

         for( auto& connection : connections)
         {
            // send
            tcp::message::Complete complete{ 
               common::message::Type::process_lookup_request, 
               correlation, 
               payload};

            EXPECT_TRUE( device::blocking::send( connection, std::move( complete)));
         }

         // receive
         for( auto& connection : connections)
         {
            auto complete = device::blocking::next( connection);

            EXPECT_TRUE( complete);
            EXPECT_TRUE( complete.correlation() == correlation);
            EXPECT_TRUE( complete.type() == common::message::Type::process_lookup_request);
            EXPECT_TRUE( complete.payload == payload) << "complete: " << complete;
         }
      }

      TEST( common_communication_tcp, echo_server_port__tcp_device_send_receive__expect_connection)
      {
         common::unittest::Trace trace;

         const auto address = local::address();
         auto server_scope = local::spawn_tcp_server( address);

         tcp::Duplex tcp{ local::connect( address)};

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
         auto server_scope = local::spawn_tcp_server( address);

         tcp::Duplex tcp{ local::connect( address)};

         auto send_message = unittest::message::transport::size( 10 * 1024);

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
         auto server_scope = local::spawn_tcp_server( address);

         tcp::Duplex tcp{ local::connect( address)};

         auto send_message = unittest::message::transport::size( 100 * 1024);

         auto correlation = device::blocking::send( tcp, send_message);


         // receive (the echo)
         {
            unittest::Message receive_message;
            device::blocking::receive( tcp, receive_message, correlation);

            EXPECT_TRUE( common::algorithm::equal( receive_message.payload, send_message.payload));
         }
      }

      TEST( common_communication_tcp, echo_server_port__tcp_device_send_receive__1M_payload)
      {
         common::unittest::Trace trace;

         const auto address = local::address();
         auto server_scope = local::spawn_tcp_server( address);

         tcp::Duplex tcp{ local::connect( address)};

         auto send_message = unittest::message::transport::size( 1024 * 1024);;

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
         auto server_scope = local::spawn_tcp_server( address);

         tcp::Duplex tcp{ local::connect( address)};

         auto send_message = unittest::message::transport::size( 10 * 1024 * 1024);
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
