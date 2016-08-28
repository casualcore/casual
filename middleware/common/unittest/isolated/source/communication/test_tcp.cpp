//!
//! casual 
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "common/communication/tcp.h"



#include "common/message/service.h"
#include "common/mockup/thread.h"

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
               void simple_server( std::string port)
               {
                  try
                  {
                     tcp::Listener listener{ tcp::Address::Port{ port}};

                     std::vector< tcp::Socket> connections;

                     while( true)
                     {
                        connections.push_back( listener());

                        log::debug << "connections: " << range::make( connections) << std::endl;

                     }
                  }
                  catch( ...)
                  {
                     common::error::handler();
                  }
               }

               template< typename T>
               bool boolean( T&& value)
               {
                  return static_cast< bool>( value);
               }
            } // <unnamed>
         } // local

         TEST( casual_common_communication_tcp, connect_to_non_existent_port__expect_connection_refused)
         {   
            common::unittest::Trace trace;

            EXPECT_THROW( {
               tcp::connect( tcp::Address::Port{ "23666"});
            }, exception::communication::Refused);
         }

         TEST( casual_common_communication_tcp, listener_port_23666)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW({
               tcp::Listener listener{ tcp::Address::Port{ "23666"}};
            });
         }

         TEST( casual_common_communication_tcp, connect_to_listener_on_localhost__expect_correct_info)
         {
            common::unittest::Trace trace;

            //const std::string host{ "127.0.0.1"};
            const std::string host{ "localhost"};
            const std::string port{ "6666"};

            mockup::Thread server{ &local::simple_server, port};

            const auto socket =
                  tcp::retry::connect(
                        { tcp::Address::Host{ host}, tcp::Address::Port{ port}},
                        { { std::chrono::milliseconds{ 1}, 0}});

            {
               const auto client = tcp::socket::address::host( socket.descriptor());
               EXPECT_TRUE( client.host == host) << client.host;
            }

            {
               const auto server = tcp::socket::address::peer( socket.descriptor());
               EXPECT_TRUE( server.host == host) << server.host;
               EXPECT_TRUE( server.port == port) << server.port;
            }
         }

         TEST( casual_common_communication_tcp, listener_port_23666__connect_to_port__expect_connection)
         {
            common::unittest::Trace trace;

            mockup::Thread server{ &local::simple_server, std::string{ "23666"}};

            auto socket = tcp::retry::connect( tcp::Address::Port{ "23666"}, { { std::chrono::milliseconds{ 1}, 0}});

            EXPECT_TRUE( local::boolean( socket));
         }


         TEST( casual_common_communication_tcp, listener_port_23666__connect_to_port_10_times__expect_connections)
         {
            common::unittest::Trace trace;

            mockup::Thread server{ &local::simple_server, std::string{ "23666"}};

            std::vector< tcp::Socket> connections;

            for( int count = 0; count < 10; ++count)
            {
               connections.push_back( tcp::retry::connect( tcp::Address::Port{ "23666"}, { { std::chrono::milliseconds{ 1}, 0}}));
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
                           tcp::message::Transport transport;

                           tcp::native::receive( socket, transport, {});
                           tcp::native::send( socket, transport, {});
                        }
                     }
                     catch( ...)
                     {
                        common::error::handler();
                     }

                  }

                  void server( std::string port)
                  {
                     std::vector< mockup::Thread> workers;

                     try
                     {
                        tcp::Listener listener{ tcp::Address::Port{ port}};

                        while( true)
                        {
                           workers.emplace_back( &echo::worker, listener());
                        }
                     }
                     catch( ...)
                     {
                        common::error::handler();
                     }
                  }

               } // echo
            } // <unnamed>
         } // local


         TEST( casual_common_communication_tcp, echo_server_port_23666__connect_to_port__expect_connection)
         {
            common::unittest::Trace trace;

            const std::string port{ "23666"};

            mockup::Thread server{ &local::echo::server, port};

            auto socket = tcp::retry::connect( tcp::Address::Port{ port}, { { std::chrono::milliseconds{ 1}, 0}});

            EXPECT_TRUE( local::boolean( socket));

            platform::binary_type payload{ 1,2,3,4,5,6,7,8,9};
            auto correlation = uuid::make();

            // send
            {
               tcp::message::Transport transport{ common::message::Type::process_lookup_request};
               correlation.copy( transport.message.header.correlation);
               transport.assign( std::begin( payload), std::end( payload));

               EXPECT_TRUE( tcp::native::send( socket, transport, {}));
            }

            // receive
            {
               tcp::message::Transport transport;

               EXPECT_TRUE( tcp::native::receive( socket, transport, {}));
               EXPECT_TRUE( transport.message.header.correlation == correlation);
               EXPECT_TRUE( transport.type() == common::message::Type::process_lookup_request);
               auto payload_range = range::make( std::begin( transport.message.payload), transport.message.header.count);
               EXPECT_TRUE( range::equal( payload, payload_range)) << "payload: " << transport;
            }
         }

         TEST( casual_common_communication_tcp, echo_server_port_23666__10_connect_to_port__expect_echo_from_10)
         {
            common::unittest::Trace trace;

            const std::string port{ "23666"};

            mockup::Thread server{ &local::echo::server, port};

            std::vector< tcp::Socket> connections( 10);

            for( auto& socket : connections)
            {
               socket = tcp::retry::connect( tcp::Address::Port{ port}, { { std::chrono::milliseconds{ 1}, 0}});
               EXPECT_TRUE( local::boolean( socket));
            }


            platform::binary_type payload{ 1,2,3,4,5,6,7,8,9};
            auto correlation = uuid::make();

            for( auto& socket : connections)
            {
               // send
               tcp::message::Transport transport{ common::message::Type::process_lookup_request};
               correlation.copy( transport.message.header.correlation);
               transport.assign( std::begin( payload), std::end( payload));

               EXPECT_TRUE( tcp::native::send( socket, transport, {}));
            }

            // receive
            for( auto& socket : connections)
            {
               tcp::message::Transport transport;

               EXPECT_TRUE( tcp::native::receive( socket, transport, {}));
               EXPECT_TRUE( transport.message.header.correlation == correlation);
               EXPECT_TRUE( transport.type() == common::message::Type::process_lookup_request);
               auto payload_range = range::make( std::begin( transport.message.payload), transport.message.header.count);
               EXPECT_TRUE( range::equal( payload, payload_range)) << "payload: " << transport;
            }
         }

         TEST( casual_common_communication_tcp, echo_server_port_23666__tcp_device_send_receive__expect_connection)
         {
            common::unittest::Trace trace;

            const std::string port{ "23666"};

            mockup::Thread server{ &local::echo::server, port};

            tcp::outbound::Device outbund{ tcp::retry::connect( tcp::Address::Port{ port}, { { std::chrono::milliseconds{ 1}, 0}})};

            EXPECT_TRUE( local::boolean( socket));

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

      } // communication
   } // common


} // casual
