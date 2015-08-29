//!
//! casual_isolatedunittest_broker.cpp
//!
//! Created on: May 5, 2012
//!     Author: Lazan
//!




#include <gtest/gtest.h>

#include "broker/broker.h"
#include "broker/handle.h"
#include "broker/admin/server.h"

#include "common/mockup/ipc.h"
#include "common/message/type.h"


namespace casual
{

   using namespace common;

	namespace broker
	{
	   namespace local
      {
         namespace
         {





            struct Broker
            {
               Broker( broker::State& state) : m_thread{
                  &broker::message::pump, std::ref( state), std::ref( queue)
               }
               {

               }


               ~Broker()
               {
                  common::queue::blocking::Send send;
                  // make sure we quit
                  send( queue.id(), common::message::shutdown::Request{});
                  m_thread.join();
               }

               common::ipc::receive::Queue queue;

            private:

               std::thread m_thread;

            };

            struct domain_0
            {

               void add_instance( const std::string& alias, platform::pid_type pid)
               {
                  state::Server server;
                  server.alias = alias;
                  server.configured_instances = 1;
                  server.path = "some/path/" + alias;
                  server.instances.push_back( pid);
                  state.add( std::move( server));

                  state::Server::Instance instance;
                  instance.process.pid = pid;
                  instance.server = server.id;

                  state.add( std::move( instance));
               }

               broker::State state;
            };

            struct domain_1 : domain_0
            {
               domain_1() : server1{ 10}
               {
                  add_instance( "server1", server1.process().pid);
               }

               mockup::ipc::Instance server1;

            };

            struct domain_2 : domain_1
            {
               domain_2()
               {
                  auto& instance = state.getInstance( server1.process().pid);
                  instance.process = server1.process();
               }
            };

            struct domain_3 : domain_2
            {
               domain_3() : server2{ 20}
               {
                  auto& instance = state.getInstance( server1.process().pid);
                  instance.state = state::Server::Instance::State::idle;

                  {
                     auto& service = state.add( state::Service{ "service1"});
                     service.instances.emplace_back( instance);
                     instance.services.emplace_back( service);
                  }

                  {
                     auto& service = state.add( state::Service{ "service2"});
                     service.instances.emplace_back( instance);
                     instance.services.emplace_back( service);
                  }
               }

               state::Server::Instance& instance1() { return state.getInstance( server1.process().pid);}

               mockup::ipc::Instance server2;
            };


            struct domain_4 : domain_3
            {
               domain_4() : forward{ 40}
               {
                  state.forward = forward.process();
               }

               mockup::ipc::Instance forward;

            };

            struct domain_5 : domain_3
            {
               domain_5() : tm{ 50}
               {
                  add_instance( "tm", tm.process().pid);
               }

               mockup::ipc::Instance tm;

            };


            struct domain_6
            {
               domain_6() : traffic1{ 10}, traffic2{ 20}
               {

               }

               mockup::ipc::Instance traffic1;
               mockup::ipc::Instance traffic2;

               broker::State state;

            };

            struct domain_singleton : domain_0
            {
               domain_singleton() : server1{ 1000}, server2{ 2000}
               {
                  add_instance( "server1", server1.process().pid);
                  add_instance( "server2", server2.process().pid);
               }
               mockup::ipc::Instance server1;
               mockup::ipc::Instance server2;

            };

         } // <unnamed>
      } // local


      TEST( casual_broker, admin_services)
      {
         broker::State state;

         auto arguments = broker::admin::services( state);

         EXPECT_TRUE( arguments.services.at( 0).origin == ".casual.broker.state");
      }


      TEST( casual_broker, shutdown)
      {
         EXPECT_NO_THROW({
            broker::State state;
            local::Broker broker{ state};
         });

      }

      TEST( casual_broker, server_connect)
      {
         common::Trace trace{ "casual_broker.casual_broker", common::log::internal::debug};

         local::domain_1 domain;

         {
            local::Broker broker{ domain.state};

            common::message::server::connect::Request request;
            request.process = domain.server1.process();
            request.path = "/server/path";

            common::queue::blocking::Send send;
            send( broker.queue.id(), request);
         }

         {
            common::message::server::connect::Reply reply;
            common::queue::blocking::Reader receive{ domain.server1.output()};

            receive( reply);

         }

         auto& instance = domain.state.getInstance( domain.server1.process().pid);
         EXPECT_TRUE( instance.process.queue == domain.server1.process().queue);

      }

      TEST( casual_broker, server_connect_services)
      {
         local::domain_1 domain;

         {
            local::Broker broker{ domain.state};

            common::message::server::connect::Request request;
            request.process = domain.server1.process();
            request.path = "/server/path";

            request.services = { { "service1"}, { "service2"}};

            common::queue::blocking::Send send;
            send( broker.queue.id(), request);
         }

         {
            common::message::server::connect::Reply reply;
            common::queue::blocking::Reader receive{ domain.server1.output()};

            receive( reply);

         }

         auto& instance = domain.state.getInstance( domain.server1.process().pid);
         EXPECT_TRUE( instance.process.queue == domain.server1.process().queue);

         EXPECT_NO_THROW({
            domain.state.getService( "service1");
         });
      }



		TEST( casual_broker, advertise_new_services_current_server)
      {
         local::domain_2 domain;

         {
            local::Broker broker{ domain.state};

            common::message::service::Advertise request;
            request.process = domain.server1.process();

            request.services = { { "service1"}, { "service2"}};

            common::queue::blocking::Send send;
            send( broker.queue.id(), request);
         }

         EXPECT_NO_THROW({
            domain.state.getService( "service1");
            domain.state.getService( "service2");
         });
      }



		TEST( casual_broker, unadvertise_service)
      {
         local::domain_3 domain;

         EXPECT_TRUE( domain.state.getService( "service1").instances.size() == 1);
         EXPECT_TRUE( domain.state.getService( "service2").instances.size() == 1);

         {
            local::Broker broker{ domain.state};

            common::message::service::Unadvertise request;
            request.process = domain.server1.process();

            request.services = { { "service1"}};

            common::queue::blocking::Send send;
            send( broker.queue.id(), request);
         }

         EXPECT_TRUE( domain.state.getService( "service1").instances.empty());
         EXPECT_TRUE( domain.state.getService( "service2").instances.size() == 1);

      }

      TEST( casual_broker, service_lookup_non_existent__expect_absent_reply)
      {
         local::domain_3 domain;

         {
            local::Broker broker{ domain.state};

            common::message::service::lookup::Request request;
            request.process = domain.server2.process();
            request.requested = "non_existent";

            common::queue::blocking::Send send;
            auto correlation = send( broker.queue.id(), request);

            common::queue::blocking::Reader receive{ domain.server2.output()};
            common::message::service::lookup::Reply reply;
            receive( reply);

            EXPECT_TRUE( correlation == reply.correlation);
            EXPECT_FALSE( static_cast< bool>( reply.process));
            EXPECT_TRUE( reply.state == common::message::service::lookup::Reply::State::absent);
         }
      }


		TEST( casual_broker, service_lookup__expect_idle_reply)
      {
         local::domain_3 domain;

         {
            local::Broker broker{ domain.state};

            common::message::service::lookup::Request request;
            request.process = domain.server2.process();
            request.requested = "service1";

            common::queue::blocking::Send send;
            auto correlation = send( broker.queue.id(), request);

            common::queue::blocking::Reader receive{ domain.server2.output()};
            common::message::service::lookup::Reply reply;
            receive( reply);

            EXPECT_TRUE( correlation == reply.correlation);
            EXPECT_TRUE( reply.process == domain.server1.process());
            EXPECT_TRUE( reply.state == common::message::service::lookup::Reply::State::idle);
         }
      }


      TEST( casual_broker, service_lookup_service1__expect__busy_reply__pending_reply)
      {
         local::domain_3 domain;
         domain.instance1().state = state::Server::Instance::State::busy;

         {
            local::Broker broker{ domain.state};

            common::message::service::lookup::Request request;
            request.process = domain.server2.process();
            request.requested = "service1";

            common::queue::blocking::Send send;
            auto correlation = send( broker.queue.id(), request);

            common::queue::blocking::Reader receive{ domain.server2.output()};
            common::message::service::lookup::Reply reply;
            receive( reply);

            EXPECT_TRUE( correlation == reply.correlation);
            EXPECT_FALSE( static_cast< bool>( reply.process)) << "process: " <<  reply.process;
            EXPECT_TRUE( reply.state == common::message::service::lookup::Reply::State::busy);
         }
         ASSERT_TRUE( domain.state.pending.requests.size() == 1);
         EXPECT_TRUE( domain.state.pending.requests.at( 0).process == domain.server2.process());
      }


      TEST( casual_broker, service_lookup_service1__forward_context___expect__forward_reply)
      {
         local::domain_4 domain;
         domain.instance1().state = state::Server::Instance::State::busy;

         {
            local::Broker broker{ domain.state};

            common::message::service::lookup::Request request;
            request.process = domain.server2.process();
            request.requested = "service1";
            request.context =  common::message::service::lookup::Request::Context::no_reply;

            common::queue::blocking::Send send;
            auto correlation = send( broker.queue.id(), request);

            common::queue::blocking::Reader receive{ domain.server2.output()};
            common::message::service::lookup::Reply reply;
            receive( reply);

            EXPECT_TRUE( correlation == reply.correlation);
            EXPECT_TRUE( reply.process == domain.state.forward) << "process: " <<  reply.process;
            EXPECT_TRUE( reply.state == common::message::service::lookup::Reply::State::idle);
         }
         EXPECT_TRUE( domain.state.pending.requests.empty());
      }


      TEST( casual_broker, service_lookup_service1__expect__busy_reply__pending_reply__service_ACK__expect_reply)
      {
         auto start_time = platform::clock_type::now();

         local::domain_3 domain;
         domain.instance1().alterState( state::Server::Instance::State::busy);

         Uuid correlation;

         {
            local::Broker broker{ domain.state};

            common::message::service::lookup::Request request;
            request.process = domain.server2.process();
            request.requested = "service1";

            common::queue::blocking::Send send;
            correlation = send( broker.queue.id(), request);

            common::queue::blocking::Reader receive{ domain.server2.output()};
            common::message::service::lookup::Reply reply;
            receive( reply);

            EXPECT_TRUE( correlation == reply.correlation);
            EXPECT_FALSE( static_cast< bool>( reply.process)) << "process: " <<  reply.process;
            EXPECT_TRUE( reply.state == common::message::service::lookup::Reply::State::busy);
         }

         ASSERT_TRUE( domain.state.pending.requests.size() == 1);
         EXPECT_TRUE( domain.state.pending.requests.at( 0).process == domain.server2.process());

         {
            local::Broker broker{ domain.state};

            common::message::service::call::ACK ack;
            ack.process = domain.server1.process();
            ack.service = "service1";

            common::queue::blocking::Send send;
            send( broker.queue.id(), ack);

            //
            // Expect an "idle" lookup-reply
            //
            common::queue::blocking::Reader receive{ domain.server2.output()};
            common::message::service::lookup::Reply reply;
            receive( reply);

            EXPECT_TRUE( correlation == reply.correlation);
            EXPECT_TRUE( reply.process == domain.server1.process()) << "process: " <<  reply.process;
            EXPECT_TRUE( reply.state == common::message::service::lookup::Reply::State::idle);
         }

         auto end_time = platform::clock_type::now();
         EXPECT_TRUE( domain.instance1().invoked == 1);
         EXPECT_TRUE( domain.instance1().last >= start_time);
         EXPECT_TRUE( domain.instance1().last <= end_time);

      }

      TEST( casual_broker, forward_connect)
      {
         local::domain_3 domain;

         {
            local::Broker broker{ domain.state};

            common::message::forward::connect::Request connect;
            connect.process = domain.server2.process();

            common::queue::blocking::Send send;
            send( broker.queue.id(), connect);
         }
         EXPECT_TRUE( domain.state.forward == domain.server2.process());
      }


		TEST( casual_broker, traffic_connect)
      {
         local::domain_6 domain;

         {
            local::Broker broker{ domain.state};

            common::message::traffic::monitor::connect::Request connect;
            connect.process = domain.traffic1.process();

            common::queue::blocking::Send send;
            send( broker.queue.id(), connect);
         }
         EXPECT_TRUE( domain.state.traffic.monitors.at( 0) == domain.traffic1.process().queue);
      }

      TEST( casual_broker, traffic_connect_x2__expect_2_traffic_monitors)
      {
         local::domain_6 domain;

         {
            local::Broker broker{ domain.state};

            {
               common::message::traffic::monitor::connect::Request connect;
               connect.process = domain.traffic1.process();

               common::queue::blocking::Send send;
               send( broker.queue.id(), connect);
            }
            {
               common::message::traffic::monitor::connect::Request connect;
               connect.process = domain.traffic2.process();

               common::queue::blocking::Send send;
               send( broker.queue.id(), connect);
            }

         }
         EXPECT_TRUE( domain.state.traffic.monitors.at( 0) == domain.traffic1.process().queue);
         EXPECT_TRUE( domain.state.traffic.monitors.at( 1) == domain.traffic2.process().queue);
      }

		TEST( casual_broker, traffic_disconnect)
      {
         local::domain_3 domain;
         domain.state.traffic.monitors.push_back( domain.server2.process().queue);

         {
            local::Broker broker{ domain.state};

            common::message::traffic::monitor::Disconnect disconnect;
            disconnect.process = domain.server2.process();

            common::queue::blocking::Send send;
            send( broker.queue.id(), disconnect);
         }
         EXPECT_TRUE( domain.state.traffic.monitors.empty());
      }


      TEST( casual_broker, tm_connect_ready)
      {
         local::domain_5 domain;

         {
            local::Broker broker{ domain.state};

            common::message::transaction::manager::connect::Request connect;
            connect.process = domain.tm.process();

            common::queue::blocking::Send send;
            send( broker.queue.id(), connect);

            common::message::transaction::manager::Ready ready;
            ready.process = domain.tm.process();
            ready.success = true;
            send( broker.queue.id(), ready);
         }
         EXPECT_TRUE( domain.state.transaction_manager == domain.tm.process().queue);
         EXPECT_TRUE( domain.state.getInstance( domain.tm.process().pid).state == state::Server::Instance::State::idle);
      }

      TEST( casual_broker, singelton_connect__expect_stored_uuid)
      {
         local::domain_5 domain;

         auto& tm_uuid = common::process::instance::identity::transaction::manager();
         {

            local::Broker broker{ domain.state};

            common::message::transaction::manager::connect::Request connect;
            connect.process = domain.tm.process();
            connect.identification = tm_uuid;

            common::queue::blocking::Send send;
            send( broker.queue.id(), connect);

         }
         EXPECT_TRUE( domain.state.singeltons.size() == 1);
         EXPECT_TRUE( domain.state.singeltons.at( tm_uuid) == domain.tm.process());
      }

      TEST( casual_broker, singelton_connect_2x__expect__connect_reply_error)
      {
         local::domain_singleton domain;

         {
            auto some_uuid = common::uuid::make();
            local::Broker broker{ domain.state};

            common::message::server::connect::Request connect;
            {
               connect.process = domain.server1.process();
               connect.identification = some_uuid;

               common::queue::blocking::Send send;
               send( broker.queue.id(), connect);
            }
            {
               connect.process = domain.server2.process();
               connect.identification = some_uuid;

               common::queue::blocking::Send send;
               send( broker.queue.id(), connect);
            }
            common::message::server::connect::Reply reply;

            {
               common::queue::blocking::Reader reader{ domain.server1.output()};
               reader( reply);
               EXPECT_TRUE( reply.directive == common::message::server::connect::Reply::Directive::start);

            }
            {
               common::queue::blocking::Reader reader{ domain.server2.output()};
               reader( reply);
               EXPECT_TRUE( reply.directive == common::message::server::connect::Reply::Directive::singleton);
            }
         }
      }

      TEST( casual_broker, singelton_connect__lookup_process__expect_reply)
      {
         local::domain_singleton domain;

         {
            auto some_uuid = common::uuid::make();
            local::Broker broker{ domain.state};

            {
               common::message::server::connect::Request connect;
               connect.process = domain.server1.process();
               connect.identification = some_uuid;

               common::queue::blocking::Send send;
               send( broker.queue.id(), connect);
            }


            {
               common::message::server::connect::Reply reply;

               common::queue::blocking::Reader reader{ domain.server1.output()};
               reader( reply);
               EXPECT_TRUE( reply.directive == common::message::server::connect::Reply::Directive::start);
            }

            {
               common::message::lookup::process::Request request;
               request.identification = some_uuid;
               request.process = domain.server2.process();

               common::queue::blocking::Send send;
               send( broker.queue.id(), request);

               common::message::lookup::process::Reply reply;
               common::queue::blocking::Reader reader{ domain.server2.output()};
               reader( reply);
               EXPECT_TRUE( reply.process == domain.server1.process());
            }
         }
      }

      TEST( casual_broker, lookup_process__direct__expect_reply_with_null_process)
      {
         local::domain_singleton domain;

         {
            auto some_uuid = common::uuid::make();
            local::Broker broker{ domain.state};

            {
               common::message::lookup::process::Request request;
               request.directive = common::message::lookup::process::Request::Directive::direct;
               request.identification = some_uuid;
               request.process = domain.server2.process();

               common::queue::blocking::Send send;
               send( broker.queue.id(), request);

               common::message::lookup::process::Reply reply;
               common::queue::blocking::Reader reader{ domain.server2.output()};
               reader( reply);
               EXPECT_TRUE( reply.process.pid == 0);
               EXPECT_TRUE( reply.process.queue == 0);

            }
         }
      }

      TEST( casual_broker, lookup_process_wait__expect_pending_reply)
      {
         local::domain_singleton domain;

         auto some_uuid = common::uuid::make();

         {
            local::Broker broker{ domain.state};

            {
               common::message::lookup::process::Request request;
               request.directive = common::message::lookup::process::Request::Directive::wait;
               request.identification = some_uuid;
               request.process = domain.server2.process();

               common::queue::blocking::Send send;
               send( broker.queue.id(), request);
            }
         }
         ASSERT_TRUE( domain.state.pending.process_lookup.size() == 1);
         EXPECT_TRUE( domain.state.pending.process_lookup.front().identification == some_uuid);
         EXPECT_TRUE( domain.state.pending.process_lookup.front().process == domain.server2.process());
      }

      TEST( casual_broker, lookup_process_wait__singleton_connect__expect_reply)
      {
         local::domain_singleton domain;

         {
            auto some_uuid = common::uuid::make();
            local::Broker broker{ domain.state};



            {
               common::message::lookup::process::Request request;
               request.identification = some_uuid;
               request.process = domain.server2.process();

               common::queue::blocking::Send send;
               send( broker.queue.id(), request);
            }

            {
               common::message::server::connect::Request connect;
               connect.process = domain.server1.process();
               connect.identification = some_uuid;

               common::queue::blocking::Send send;
               send( broker.queue.id(), connect);

               common::message::server::connect::Reply reply;

               common::queue::blocking::Reader reader{ domain.server1.output()};
               reader( reply);
               EXPECT_TRUE( reply.directive == common::message::server::connect::Reply::Directive::start);
            }

            {
               common::message::lookup::process::Reply reply;
               common::queue::blocking::Reader reader{ domain.server2.output()};
               reader( reply);
               EXPECT_TRUE( reply.process == domain.server1.process());

            }
         }
      }
	}
}
