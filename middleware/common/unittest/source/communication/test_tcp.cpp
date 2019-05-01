//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include <gtest/gtest.h>
#include "common/unittest.h"
#include "common/unittest/thread.h"

#include "common/communication/tcp.h"
#include "common/exception/system.h"
#include "common/exception/handle.h"

#include "common/message/service.h"


namespace casual
{
   namespace common
   {
      namespace communication
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

                     std::vector< tcp::Socket> connections;

                     while( true)
                     {
                        connections.push_back( listener());

                        log::line( log::debug, "connections: ", connections);

                     }
                  }
                  catch( const exception::signal::exception& exception)
                  {
                     log::line( log::debug, "got signal: ", exception);
                  }
                  catch( ...)
                  {
                     exception::handle();
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
                  return { { "127.0.0.1"}, { ++port}};
               }

            } // <unnamed>
         } // local

         TEST( casual_common_communication_tcp, connect_to_non_existent_port__expect_connection_refused)
         {   
            common::unittest::Trace trace;

            EXPECT_THROW( {
               tcp::connect( local::address());
            }, exception::system::communication::Refused);
         }

         TEST( casual_common_communication_tcp, listener_port)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW({
               tcp::Listener listener{ local::address()};
            });
         }


         TEST( casual_common_communication_tcp, listener_port__connect_to_port__expect_connection)
         {
            common::unittest::Trace trace;

            const auto address = local::address();

            unittest::Thread server{ &local::simple_server, address};

            auto socket = tcp::retry::connect( address, { { std::chrono::milliseconds{ 1}, 0}});

            EXPECT_TRUE( local::boolean( socket));
         }


         TEST( casual_common_communication_tcp, listener_port__connect_to_port_10_times__expect_connections)
         {
            common::unittest::Trace trace;

            const auto address = local::address();

            unittest::Thread server{ &local::simple_server, address};

            std::vector< tcp::Socket> connections;

            for( int count = 0; count < 10; ++count)
            {
               connections.push_back( tcp::retry::connect( address, { { std::chrono::milliseconds{ 1}, 0}}));
            }

            for( auto& s : connections)
            {
               EXPECT_TRUE( local::boolean( s)) << "socket: " << s;
            }
         }

         namespace local
         {
            namespace
            {
               namespace echo
               {
                  void worker( tcp::Socket&& socket)
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
                        exception::handle();
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
                        exception::handle();
                     }
                  }

               } // echo
            } // <unnamed>
         } // local


         TEST( casual_common_communication_tcp, echo_server_port__connect_to_port__expect_connection)
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
               communication::message::Complete message{ common::message::Type::process_lookup_request, correlation};
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

         TEST( casual_common_communication_tcp, echo_server_port__10_connect_to_port__expect_echo_from_10)
         {
            common::unittest::Trace trace;

            const auto address = local::address();

            unittest::Thread server{ &local::echo::server, address};

            std::vector< tcp::Socket> connections( 10);

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
               communication::message::Complete message{ common::message::Type::process_lookup_request, correlation};
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

         TEST( casual_common_communication_tcp, echo_server_port__tcp_device_send_receive__expect_connection)
         {
            common::unittest::Trace trace;

            const auto address = local::address();

            unittest::Thread server{ &local::echo::server, address};

            tcp::outbound::Device outbund{ tcp::retry::connect( address, { { std::chrono::milliseconds{ 1}, 0}})};


            auto send = [&](){
               common::message::service::lookup::Request message;
               message.process = process::handle();
               message.requested = "testservice";

               return outbund.blocking_send( message);
            };


            auto correlation = send();


            // receive (the echo)
            {
               common::message::service::lookup::Request message;
               tcp::inbound::Device tcp{ outbund.connector().socket()};

               tcp.receive( message, correlation, tcp::inbound::Device::blocking_policy{});

               EXPECT_TRUE( message.process == process::handle());
               EXPECT_TRUE( message.requested == "testservice");
            }
         }


         TEST( casual_common_communication_tcp, echo_server_port__tcp_device_send_receive__10k_payload)
         {
            common::unittest::Trace trace;

            const auto address = local::address();

            unittest::Thread server{ &local::echo::server, address};

            tcp::outbound::Device outbund{ tcp::retry::connect( address, { { std::chrono::milliseconds{ 1}, 0}})};

            auto send_message = unittest::random::message( 10 * 1024);

            auto correlation = outbund.blocking_send( send_message);


            // receive (the echo)
            {
               unittest::Message receive_message;
               tcp::inbound::Device tcp{ outbund.connector().socket()};

               tcp.receive( receive_message, correlation, tcp::inbound::Device::blocking_policy{});

               EXPECT_TRUE( common::algorithm::equal( receive_message.payload, send_message.payload));
            }
         }

         TEST( casual_common_communication_tcp, echo_server_port__tcp_device_send_receive_100k_payload)
         {
            common::unittest::Trace trace;

            const auto address = local::address();

            unittest::Thread server{ &local::echo::server, address};

            tcp::outbound::Device outbund{ tcp::retry::connect( address, { { std::chrono::milliseconds{ 1}, 0}})};

            auto send_message = unittest::random::message( 100 * 1024);

            auto correlation = outbund.blocking_send( send_message);


            // receive (the echo)
            {
               unittest::Message receive_message;
               tcp::inbound::Device tcp{ outbund.connector().socket()};

               tcp.receive( receive_message, correlation, tcp::inbound::Device::blocking_policy{});

               EXPECT_TRUE( common::algorithm::equal( receive_message.payload, send_message.payload));
            }
         }

         TEST( casual_common_communication_tcp, echo_server_port__tcp_device_send_receive__1M_paylad)
         {
            common::unittest::Trace trace;

            const auto address = local::address();

            unittest::Thread server{ &local::echo::server, address};

            tcp::outbound::Device outbund{ tcp::retry::connect( address, { { std::chrono::milliseconds{ 1}, 0}})};

            auto send_message = unittest::random::message( 1024 * 1024);;

            auto correlation = outbund.blocking_send( send_message);


            // receive (the echo)
            {
               unittest::Message receive_message;
               tcp::inbound::Device tcp{ outbund.connector().socket()};

               tcp.receive( receive_message, correlation, tcp::inbound::Device::blocking_policy{});

               EXPECT_TRUE( common::algorithm::equal( receive_message.payload, send_message.payload));
            }
         }


         TEST( casual_common_communication_tcp, echo_server_port__tcp_device_send_receive__10M_payload)
         {
            common::unittest::Trace trace;

            const auto address = local::address();

            unittest::Thread server{ &local::echo::server, address};

            tcp::outbound::Device outbund{ tcp::retry::connect( address, { { std::chrono::milliseconds{ 1}, 0}})};


            auto send_message = unittest::random::message( 10 * 1024 * 1024);
            unittest::random::range( send_message.payload);

            std::cerr << "sending outbund " << '\n';

            auto correlation = outbund.blocking_send( send_message);


            // receive (the echo)
            {
               std::cerr << "receiving outbund " << '\n';

               unittest::Message receive_message;
               tcp::inbound::Device tcp{ outbund.connector().socket()};

               tcp.receive( receive_message, correlation, tcp::inbound::Device::blocking_policy{});

               EXPECT_TRUE( common::algorithm::equal( receive_message.payload, send_message.payload));
            }
         }
      } // communication
   } // common
} // casual
