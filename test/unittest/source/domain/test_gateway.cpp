//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

// to be able to use 'raw' flags and codes
// since we undefine 'all' of them in common
#define CASUAL_NO_XATMI_UNDEFINE

#include "common/unittest.h"

#include "domain/manager/unittest/process.h"
#include "domain/manager/admin/cli.h"

#include "common/communication/instance.h"
#include "serviceframework/service/protocol/call.h"

#include "gateway/manager/admin/model.h"
#include "gateway/manager/admin/server.h"

#include "service/manager/admin/server.h"
#include "service/manager/admin/model.h"

#include "queue/api/queue.h"

#include "casual/xatmi.h"


#include <regex>

namespace casual
{
   using namespace common;

   namespace test::domain
   {

      namespace local
      {
         namespace
         {
            using Manager = casual::domain::manager::unittest::Process;

            namespace configuration
            {
               constexpr auto base = R"(
domain: 
   groups: 
      -  name: base
      -  name: queue
         dependencies: [ base]
      -  name: user
         dependencies: [ queue]
      -  name: gateway
         dependencies: [ user]
   
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/gateway/bin/casual-gateway-manager"
        memberships: [ gateway]
)";
               
            } // configuration

            namespace state
            {
               template< typename P, typename F>
               auto until( P&& predicate, F&& fetch)
               {
                  auto state = fetch();

                  auto count = 1000;

                  while( ! predicate( state) && count-- > 0)
                  {
                     process::sleep( std::chrono::milliseconds{ 2});
                     state = fetch();
                  }

                  return state;
               }

               namespace gateway
               {
                  auto call()
                  {
                     serviceframework::service::protocol::binary::Call call;
                     auto reply = call( casual::gateway::manager::admin::service::name::state);

                     casual::gateway::manager::admin::model::State result;
                     reply >> CASUAL_NAMED_VALUE( result);

                     return result;
                  }

                  template< typename P>
                  auto until( P&& predicate)
                  {
                     return state::until( std::forward< P>( predicate), []()
                     {
                        return state::gateway::call();
                     });
                  }

                  namespace predicate::outbound
                  {
                     // returns a predicate that checks if all out-connections has a 'remote id'
                     auto connected()
                     {
                        return []( auto& state)
                        {
                           return algorithm::all_of( state.connections, []( auto& connection)
                           {
                              return connection.bound != decltype( connection.bound)::out || connection.remote.id;
                           }); 
                        };
                     };

                     // returns a predicate that checks if all out-connections has NOT a 'remote id'
                     auto disconnected()
                     {
                        return []( auto& state)
                        {
                           return algorithm::all_of( state.connections, []( auto& connection)
                           {
                              return connection.bound != decltype( connection.bound)::out || ! connection.remote.id;
                           }); 
                        };
                     };
                  } // predicate::outbound

               } // gateway

            } // state

            auto allocate( platform::size::type size = 128)
            {
               auto buffer = tpalloc( X_OCTET, nullptr, size);
               unittest::random::range( range::make( buffer, size));
               return buffer;
            }

            auto call( std::string_view service, int code)
            {
               auto buffer = local::allocate( 128);
               auto len = tptypes( buffer, nullptr, nullptr);

               tpcall( service.data(), buffer, 128, &buffer, &len, 0);
               EXPECT_TRUE( tperrno == code) << "tperrno: " << tperrnostring( tperrno) << " code: " << tperrnostring( code);

               tpfree( buffer);
            };

            auto call( std::string_view service)
            {
               auto buffer = local::allocate( 128);
               auto len = tptypes( buffer, nullptr, nullptr);

               tpcall( service.data(), buffer, 128, &buffer, &len, 0);
               EXPECT_TRUE( tperrno == 0) << "tperrno: " << tperrnostring( tperrno);

               return memory::guard( buffer, &tpfree);
            };


            auto call( std::string_view service, const platform::binary::type& binary)
            {
               auto buffer = local::allocate( binary.size());
               assert( buffer);
               algorithm::copy( binary, buffer);

               auto len = tptypes( buffer, nullptr, nullptr);

               tpcall( service.data(), buffer, len, &buffer, &len, 0);
               EXPECT_TRUE( tperrno == 0) << "tperrno: " << tperrnostring( tperrno);

               return memory::guard( buffer, &tpfree);
            };

            auto acall( std::string_view service, const platform::binary::type& binary)
            {
               auto buffer = memory::guard( local::allocate( binary.size()), &tpfree);
               assert( buffer);
               algorithm::copy( binary, buffer.get());

               auto len = tptypes( buffer.get(), nullptr, nullptr);

               auto result = tpacall( service.data(), buffer.get(), len, 0);
               EXPECT_TRUE( tperrno == 0) << "tperrno: " << tperrnostring( tperrno);

               return result;
            };

            auto receive( int descriptor)
            {
               auto buffer = local::allocate( 128);
               assert( buffer);

               auto len = tptypes( buffer, nullptr, nullptr);

               tpgetrply( &descriptor, &buffer, &len, 0);
               EXPECT_TRUE( tperrno == 0) << "tperrno: " << tperrnostring( tperrno);

               platform::binary::type result;
               algorithm::copy( range::make( buffer, len), std::back_inserter( result));
               tpfree( buffer);

               return result;
            };

            template< typename B>
            auto size( B&& buffer)
            {
               return tptypes( buffer.get(), nullptr, nullptr);
            }

            template< typename T>
            void sink( T&& value)
            {
               auto sinked = std::move( value);
            }

            namespace example
            {
               auto domain( std::string name, std::string port, std::string_view extended = {})
               {
                  constexpr auto template_config = R"(
domain: 
   name: NAME

   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
        arguments: [ --sleep, 10ms]
   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:PORT
)";               
                  auto replace = []( auto text, auto pattern, auto value)
                  {
                     return std::regex_replace( text, std::regex{ pattern}, value); 
                  };

                  if( extended.empty())
                     return local::Manager{ { local::configuration::base, replace( replace( template_config, "NAME", name), "PORT", port)}};
                  else
                     return local::Manager{ { local::configuration::base, replace( replace( template_config, "NAME", name), "PORT", port), extended}};
               };

               namespace reverse
               {
                  auto domain( std::string name, std::string port, std::string_view extended = {})
                  {
                     constexpr auto template_config = R"(
domain: 
   name: NAME

   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
        arguments: [ --sleep, 10ms]
   gateway:
      reverse:
         inbound:
            groups:
               -  connections: 
                  -  address: 127.0.0.1:PORT
)";               
                     auto replace = []( auto text, auto pattern, auto value)
                     {
                        return std::regex_replace( text, std::regex{ pattern}, value); 
                     };

                     return local::Manager{ { local::configuration::base, replace( replace( template_config, "NAME", name), "PORT", port)}};
                  }
                  
               } // reverse
            } // example


            namespace domain
            {
               auto name( std::string_view name)
               {
                  return local::call( "casual/example/domain/name").get() == name;
               }

               auto name( std::string_view name, platform::size::type count)
               {
                  while( count-- > 0)
                  {
                     if( domain::name( name))
                        return true;
                     else
                        process::sleep( std::chrono::milliseconds{ 1});
                  }
                  return false;
               }
            } // domain

         } // <unnamed>
      } // local



      
      TEST( test_domain_gateway, queue_service_forward___from_A_to_B__expect_discovery)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain: 
   name: A

   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:6669
)";

         constexpr auto B = R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager"
         memberships: [ queue]
      
   queue:
      groups:
         -  alias: a
            queuebase: ':memory:'
            queues:
               -  name: a1
               -  name: a2
      forward:
         groups:
            -  services:
                  -  source: a1
                     instances: 1
                     target:
                        service: casual/example/echo
                     reply:
                        queue: a2
   
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:6669
)";

      
         // startup the domain that will try to do stuff
         local::Manager b{ { local::configuration::base, B}};
         EXPECT_TRUE( communication::instance::ping( b.handle().ipc) == b.handle());

         auto payload = unittest::random::binary( 1024);
         {
            queue::Message message;
            message.payload.type = common::buffer::type::json();
            message.payload.data = payload;
            queue::enqueue( "a1", message);
         }

         // startup the domain that has the service
         local::Manager a{ { local::configuration::base, A}};
         EXPECT_TRUE( communication::instance::ping( a.handle().ipc) == a.handle());

         // activate the b domain, so we can try to dequeue
         b.activate();

         {
            auto message = queue::blocking::dequeue( "a2");

            EXPECT_TRUE( message.payload.type == common::buffer::type::json());
            EXPECT_TRUE( message.payload.data == payload);
         }
         
      }

      TEST( test_domain_gateway, domain_A_to__B_C_D__outbound_separated_groups___expect_prio_B)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto b = local::example::domain( "B", "7001");
         auto c = local::example::domain( "C", "7002");
         auto d = local::example::domain( "D", "7003");

         constexpr auto A = R"(
domain: 
   name: A
  
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
                     services:
                        -  casual/example/domain/name
            -  connections:
                  -  address: 127.0.0.1:7002
                     services:
                        -  casual/example/domain/name
            -  connections:
                  -  address: 127.0.0.1:7003
                     services:
                        -  casual/example/domain/name
)";


         local::Manager a{ { local::configuration::base, A}};

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         // we might get to the "wrong" domain until all services are advertised.
         EXPECT_TRUE( local::domain::name( "B", 1000));

         // we expect to always get to B
         algorithm::for_n< 10>( []()
         {
            ASSERT_TRUE( local::domain::name( "B"));
         });
      }


      TEST( BACKWARD_test_domain_gateway, domain_A_to__B_C_D__outbound_separated_groups___expect_prio_B)
      {
         // same test as above but with the deprecated configuration

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         common::unittest::Trace trace;

         auto b = local::example::domain( "B", "7001");
         auto c = local::example::domain( "C", "7002");
         auto d = local::example::domain( "D", "7003");

         constexpr auto A = R"(
domain: 
   name: A
  
   gateway:
      connections:
         -  address: 127.0.0.1:7001
            services:
               -  casual/example/domain/name

         -  address: 127.0.0.1:7002
            services:
               -  casual/example/domain/name
               
         -  address: 127.0.0.1:7003
            services:
               -  casual/example/domain/name
)";


         local::Manager a{ { local::configuration::base, A}};

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         // we might get to the "wrong" domain until all services are advertised.
         EXPECT_TRUE( local::domain::name( "B", 1000));

         // we expect to always get to B
         algorithm::for_n< 10>( []()
         {
            EXPECT_TRUE( local::domain::name( "B"));
         });
      }


      TEST( test_domain_gateway, domain_A_to_B___echo_send_large_message)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto b = local::example::domain( "B", "7001");

         constexpr auto A = R"(
domain: 
   name: A
  
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
                     services:
                        -  casual/example/echo
)";


         local::Manager a{ { local::configuration::base, A}};

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         const auto binary = unittest::random::binary( 3 * platform::ipc::transport::size);

         algorithm::for_n< 10>( [&binary]()
         {
            auto buffer = local::call( "casual/example/echo", binary);
            auto size = local::size( buffer);

            EXPECT_EQ( size, range::size( binary));

            auto range = range::make( buffer.get(), size);
            EXPECT_TRUE( algorithm::equal( range, binary)) 
               << "buffer: " << transcode::hex::encode( range)
               << "\nbinary: " << transcode::hex::encode( binary);
         });
      }

      TEST( test_domain_gateway, domain_A_to_B___echo_send_message_in_transaction)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto b = local::example::domain( "B", "7001");

         constexpr auto A = R"(
domain: 
   name: A
  
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
                     services:
                        -  casual/example/echo
)";


         local::Manager a{ { local::configuration::base, A}};

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         const auto binary = unittest::random::binary( platform::ipc::transport::size);

         ASSERT_TRUE( tx_begin() == TX_OK);

         algorithm::for_n< 10>( [&binary]()
         {
            auto buffer = local::call( "casual/example/echo", binary);
            auto size = local::size( buffer);

            EXPECT_EQ( size, range::size( binary));
         });

         ASSERT_TRUE( tx_commit() == TX_OK);
      }

      TEST( test_domain_gateway, domain_A_to_B_C___call_A_B__in_same_transaction)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto b = local::example::domain( "B", "7001");
         auto c = local::example::domain( "C", "7002");

         constexpr auto A = R"(
domain: 
   name: A
  
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
                  -  address: 127.0.0.1:7002
)";


         local::Manager a{ { local::configuration::base, A}};

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         const auto binary = unittest::random::binary( platform::ipc::transport::size);

         ASSERT_TRUE( tx_begin() == TX_OK);

         algorithm::for_n< 2>( [&binary]()
         {
            {
               auto buffer = local::call( "casual/example/domain/echo/B", binary);
               auto size = local::size( buffer);

               EXPECT_EQ( size, range::size( binary));
            }
            {
               auto buffer = local::call( "casual/example/domain/echo/C", binary);
               auto size = local::size( buffer);

               EXPECT_EQ( size, range::size( binary));
            }
         });

         ASSERT_TRUE( tx_commit() == TX_OK);
      }


      TEST( test_domain_gateway, domain_A_to_B__to__C_D___call_C_D__in_same_transaction)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto c = local::example::domain( "C", "7002");
         auto d = local::example::domain( "D", "7003");

         constexpr auto B = R"(
domain: 
   name: B
  
   gateway:
      inbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001

      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7002
                     services: [ casual/example/domain/echo/C]
                  -  address: 127.0.0.1:7003
                     services: [ casual/example/domain/echo/D]
)";


         local::Manager b{ { local::configuration::base, B}};
         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         constexpr auto A = R"(
domain:
   name: A

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
)";


         local::Manager a{ { local::configuration::base, A}};
         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         const auto binary = unittest::random::binary( platform::ipc::transport::size);

         ASSERT_TRUE( tx_begin() == TX_OK);

         algorithm::for_n< 2>( [&binary]()
         {
            {
               auto buffer = local::call( "casual/example/domain/echo/C", binary);
               auto size = local::size( buffer);

               EXPECT_EQ( size, range::size( binary));
            }
            {
               auto buffer = local::call( "casual/example/domain/echo/D", binary);
               auto size = local::size( buffer);

               EXPECT_EQ( size, range::size( binary));
            }
         });

         ASSERT_TRUE( tx_commit() == TX_OK);
      }


      TEST( test_domain_gateway, domain_A_to_B__reverse_to__C_D___call_C_D__in_same_transaction)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});


         constexpr auto B = R"(
domain: 
   name: B
  
   gateway:
      inbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001

      reverse:
         outbound:
            groups:
               -  connections:
                     -  address: 127.0.0.1:7002
                        services:
                           -  casual/example/domain/echo/C
                           -  casual/example/domain/echo/D
)";


         local::Manager b{ { local::configuration::base, B}};

         auto c = local::example::reverse::domain( "C", "7002");
         auto d = local::example::reverse::domain( "D", "7002");


         constexpr auto A = R"(
domain:
   name: A

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
)";


         local::Manager a{ { local::configuration::base, A}};
         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         const auto binary = unittest::random::binary( platform::ipc::transport::size);

         ASSERT_TRUE( tx_begin() == TX_OK);

         algorithm::for_n< 2>( [&binary]()
         {
            {
               auto buffer = local::call( "casual/example/domain/echo/C", binary);
               auto size = local::size( buffer);

               EXPECT_EQ( size, range::size( binary));
            }
            {
               auto buffer = local::call( "casual/example/domain/echo/D", binary);
               auto size = local::size( buffer);

               EXPECT_EQ( size, range::size( binary));
            }
         });

         ASSERT_TRUE( tx_commit() == TX_OK);
      }

      TEST( test_domain_gateway, domain_A_to_B__to__C_D___call_C_D__from_A__call_D_from_C__in_same_transaction)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto c = local::example::domain( "C", "7002", R"(
domain:
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001

)");
         auto d = local::example::domain( "D", "7003");

         constexpr auto B = R"(
domain: 
   name: B
  
   gateway:
      inbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001

      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7002
                     services: [ casual/example/domain/echo/C]
                  -  address: 127.0.0.1:7003
                     services: [ casual/example/domain/echo/D]
)";


         local::Manager b{ { local::configuration::base, B}};
         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         constexpr auto A = R"(
domain:
   name: A

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
)";


         local::Manager a{ { local::configuration::base, A}};
         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         const auto binary = unittest::random::binary( platform::ipc::transport::size);

         ASSERT_TRUE( tx_begin() == TX_OK);

         algorithm::for_n< 2>( [&binary]()
         {
            {
               auto buffer = local::call( "casual/example/domain/echo/C", binary);
               auto size = local::size( buffer);

               EXPECT_EQ( size, range::size( binary));
            }
            {
               auto buffer = local::call( "casual/example/domain/echo/D", binary);
               auto size = local::size( buffer);

               EXPECT_EQ( size, range::size( binary));
            }
         });

         {
            // 'enter' domain C
            c.activate();

            // the outbound in C might not have been able to connect to A yet, wait for it.
            local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

            auto buffer = local::call( "casual/example/domain/echo/D", binary);
            auto size = local::size( buffer);

            EXPECT_EQ( size, range::size( binary));
         }

         ASSERT_TRUE( tx_commit() == TX_OK);
      }


      TEST( test_domain_gateway, domain_A_to_B___tpacall__sleep___shutdown_B___expect_reply___call_sleep_again__expect__TPENOENT)
      {
         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         common::unittest::Trace trace;

         local::Manager b{ { local::configuration::base, R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         arguments: [ --sleep, 100ms]
         instances: 5
   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7001
)"}};



         constexpr auto A = R"(
domain: 
   name: A

   services:
      -  name: casual/example/sleep
         routes: [ sleepy]

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
)";


         local::Manager a{ { local::configuration::base, A}};

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         const auto binary = unittest::random::binary( 2048);

         auto descriptors = algorithm::generate_n< 7>( [&binary]()
         {
            return local::acall( "sleepy", binary);
         });

         // all calls will take at least 100ms, we need to 'make sure' that the calls are 
         // in-flight. We'll sleep for 50ms to mitigate that calls did not make it to the service
         // before scaling down. 50ms should be enough for even the slowest of systems.
         // another way is to accept TPENOENT replies, but then we wouldn't really make sure
         // we test the thing we want to test.
         process::sleep( std::chrono::milliseconds{ 50});


         // shutdown B 
         local::sink( std::move( b));
         // <-- b is down.


         // receive the 7 calls
         for( auto descriptor : descriptors)
         {
            auto result = local::receive( descriptor);
            EXPECT_TRUE( result == binary);
         }

         // expect new calls to get TPENOENT
         local::call( "sleepy", TPENOENT);
      }


      TEST( test_domain_gateway, domain_A_to_B___tpacall__sleep___scale_B_down__expect__TPENOENT)
      {
         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         common::unittest::Trace trace;

         local::Manager b{ { local::configuration::base, R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         arguments: [ --sleep, 100ms]
         instances: 5
   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7001
)"}};



         constexpr auto A = R"(
domain: 
   name: A

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
)";


         local::Manager a{ { local::configuration::base, A}};

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         const auto binary = unittest::random::binary( 2048);

         auto descriptors = algorithm::generate_n< 5>( [&binary]()
         {
            return local::acall( "casual/example/sleep", binary);
         });

         // all calls will take at least 100ms, we need to 'make sure' that the calls are 
         // in-flight. We'll sleep for 50ms to mitigate that calls did not make it to the service
         // before scaling down. 50ms should be enough for even the slowest of systems.
         // another way is to accept TPENOENT replies, but then we wouldn't really make sure
         // we test the thing we want to test.
         process::sleep( std::chrono::milliseconds{ 50});
         
         {
            b.activate();

            casual::domain::manager::admin::cli cli;
            common::argument::Parse parse{ "", cli.options()};
            parse( { "domain", "--scale-instances", "casual-example-server", "0"});
         }

         a.activate();

         // receive the 10 calls
         for( auto descriptor : descriptors)
         {
            auto result = local::receive( descriptor);
            EXPECT_TRUE( result == binary);
         }

         // expect new calls to get TPENOENT
         local::call( "casual/example/sleep", TPENOENT);
      }

      TEST( test_domain_gateway, domain_A_to__B_C_D__outbound_separated_groups___expect_prio_B__shutdown_B__expect_C__shutdown__C__expect_D)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto b = local::example::domain( "B", "7001");
         auto c = local::example::domain( "C", "7002");
         auto d = local::example::domain( "D", "7003");

         constexpr auto A = R"(
domain: 
   name: A
  
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
                     services:
                        -  casual/example/domain/name
            -  connections:
                  -  address: 127.0.0.1:7002
                     services:
                        -  casual/example/domain/name
            -  connections:
                  -  address: 127.0.0.1:7003
                     services:
                        -  casual/example/domain/name
)";


         local::Manager a{ { local::configuration::base, A}};

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         // we might get to the "wrong" domain until all services are advertised.
         EXPECT_TRUE( local::domain::name( "B", 1000));

         // we expect to always get to B
         algorithm::for_n< 10>( []()
         {
            EXPECT_TRUE( local::domain::name( "B"));
         });

         // shutdown B
         local::sink( std::move( b));

         // we expect to always get to C
         algorithm::for_n< 10>( []()
         {
            EXPECT_TRUE( local::domain::name( "C"));
         });

         // shutdown C
         local::sink( std::move( c));

         // we expect to always get to D ( the only on left...)
         algorithm::for_n< 10>( []()
         {
            EXPECT_TRUE( local::domain::name( "D"));
         });
      }

      TEST( test_domain_gateway, domain_A_to__B_C__outbound_separated_groups___expect_prio_B__shutdown_B__expect_C__boot_B__expect_B)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto b = local::example::domain( "B", "7001");
         auto c = local::example::domain( "C", "7002");

         constexpr auto A = R"(
domain: 
   name: A
  
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
                     services:
                        -  casual/example/domain/name
            -  connections:
                  -  address: 127.0.0.1:7002
                     services:
                        -  casual/example/domain/name
)";


         local::Manager a{ { local::configuration::base, A}};

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         // we might get to C until all services are advertised.
         EXPECT_TRUE( local::domain::name( "B", 1000));

         // after both ar up, we expect to always get to B
         algorithm::for_n< 10>( []()
         {
            EXPECT_TRUE( local::domain::name( "B"));
         });

         // shutdown B
         local::sink( std::move( b));

         // we expect to always get to C
         algorithm::for_n< 10>( []()
         {
            EXPECT_TRUE( local::domain::name( "C"));
         });

         // boot B
         b = local::example::domain( "B", "7001");

         // we need to activate a, otherwise b has 'the control'
         a.activate();

         // we need to wait for all to be connected...
         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());
         
         // we expect to get to B agin
         EXPECT_TRUE( local::domain::name( "B", 1000));

         // we expect to always get to B, again
         algorithm::for_n< 10>( []()
         {
            EXPECT_TRUE( local::domain::name( "B"));
         });
      }

      TEST( test_domain_gateway, domain_A_to__B_C_D__outbound_same_group___expect_round_robin_between_A_B_C)
      {
         common::unittest::Trace trace;

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto b = local::example::domain( "B", "7001");
         auto c = local::example::domain( "C", "7002");
         auto d = local::example::domain( "D", "7003");

         constexpr auto A = R"(
domain: 
   name: A
  
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
                     services:
                        -  casual/example/domain/name
                  -  address: 127.0.0.1:7002
                     services:
                        -  casual/example/domain/name
                  -  address: 127.0.0.1:7003
                     services:
                        -  casual/example/domain/name
)";

         
         local::Manager a{ { local::configuration::base, A}};

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         std::map< std::string, int> domains;

         algorithm::for_n< 9>( [&domains]() mutable
         {
            auto buffer = local::call( "casual/example/domain/name");
            domains[ buffer.get()]++;
         });

         // expect even distribution
         EXPECT_TRUE( domains[ "B"] == 3);
         EXPECT_TRUE( domains[ "C"] == 3);
         EXPECT_TRUE( domains[ "D"] == 3);
      }

      TEST( test_domain_gateway, domains_A_B__B_has_echo__call_echo_from_A__expect_discovery__shutdown_B__expect_no_ent__boot_B__expect_discovery)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain: 
   name: A

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7770

)";

         constexpr auto B = R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]

   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7770

)";

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto create_domain = []( auto&& config)
         {
            return local::Manager{ { local::configuration::base, config}};
         };

         auto b = create_domain( B);
         auto a = create_domain( A);

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         local::call( "casual/example/echo", 0);

         log::line( verbose::log, "before shutdown of B");

         // "shutdown" B
         local::sink( std::move( b));

         local::state::gateway::until( local::state::gateway::predicate::outbound::disconnected());

         log::line( verbose::log, "after shutdown of B");

         local::call( "casual/example/echo", TPENOENT);

         log::line( verbose::log, "before boot of B");

         // boot B again
         b = create_domain( B);
         a.activate();

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         log::line( verbose::log, "after boot of B");

         // expect to discover echo...
         local::call( "casual/example/echo", 0);
      }

      TEST( test_domain_gateway, domains_A_B__B_has_route_b_to_echo____A_has_route_a_to_b___call_route_a_from_A___expect_discovery)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain: 
   name: A

   services:
      -  name: b
         routes: [ a]

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001

)";

         constexpr auto B = R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]

   services:
      -  name: casual/example/echo
         routes: [ b]

   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7001

)";

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto create_domain = []( auto&& config)
         {
            return local::Manager{ { local::configuration::base, config}};
         };

         auto b = create_domain( B);
         auto a = create_domain( A);

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         local::call( "a", 0);
         
      }


      TEST( test_domain_gateway, domains_A_B__B_has_echo__A_has_route_foo_to_echo__call_route_foo_from_A__expect_discovery__shutdown_B__expect_no_ent__boot_B__expect_discovery)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain: 
   name: A

   services:
      -  name: casual/example/echo
         routes: [ foo]

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7770

)";

         constexpr auto B = R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]

   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7770

)";

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto create_domain = []( auto&& config)
         {
            return local::Manager{ { local::configuration::base, config}};
         };

         auto b = create_domain( B);
         auto a = create_domain( A);

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         local::call( "foo", 0);

         log::line( verbose::log, "before shutdown of B");

         // "shutdown" B
         local::sink( std::move( b));
         local::state::gateway::until( local::state::gateway::predicate::outbound::disconnected());

         log::line( verbose::log, "after shutdown of B");

         local::call( "foo", TPENOENT);

         log::line( verbose::log, "before boot of B");

         // boot B again
         b = create_domain( B);
         a.activate();

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         log::line( verbose::log, "after boot of B");
         
         // expect to discover echo...
         local::call( "foo", 0);
      }


      TEST( test_domain_gateway, domains_A_B__B_has_echo__A_has_route_foo_and_bar_to_echo__call_route_foo_from_A__expect_discovery__shutdown_B__expect_no_ent__boot_B__expect_discovery)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain: 
   name: A

   services:
      -  name: casual/example/echo
         routes: [ foo, bar]

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7770

)";

         constexpr auto B = R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]

   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7770

)";

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto create_domain = []( auto&& config)
         {
            return local::Manager{ { local::configuration::base, config}};
         };

         auto b = create_domain( B);
         auto a = create_domain( A);

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         local::call( "foo", 0);

         log::line( verbose::log, "before shutdown of B");

         // "shutdown" B
         local::sink( std::move( b));
         local::state::gateway::until( local::state::gateway::predicate::outbound::disconnected());

         log::line( verbose::log, "after shutdown of B");

         local::call( "foo", TPENOENT);

         log::line( verbose::log, "before boot of B");

         // boot B again
         b = create_domain( B);
         a.activate();

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         log::line( verbose::log, "after boot of B");
         
         // expect to discover echo...
         local::call( "foo", 0);

      }


   } // test::domain
} // casual
