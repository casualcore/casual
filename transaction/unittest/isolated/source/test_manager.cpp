//!
//! test_manager.cpp
//!
//! Created on: Jan 6, 2014
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "transaction/manager/handle.h"
#include "transaction/manager/manager.h"


#include "common/mockup/ipc.h"
#include "common/mockup/domain.h"
#include "common/ipc.h"
#include "common/message/dispatch.h"
#include "common/environment.h"




namespace casual
{

   namespace transaction
   {


      namespace local
      {
         namespace
         {


            struct Manager
            {
               Manager( transaction::State& state)
               :
                  m_thread{ &transaction::message::pump, std::ref( state)},
                  tm_queue_link{ common::mockup::ipc::transaction::manager::queue().output().id(), common::ipc::receive::id()}
               {
               }


               ~Manager()
               {
                  common::queue::blocking::Send send;
                  // make sure we quit
                  send( common::ipc::receive::id(), common::message::shutdown::Request{});
                  m_thread.join();
               }

               struct clear_t
               {
                  clear_t()
                  {
                     common::signal::clear();
                  }
               };
            private:


               clear_t dummy;

               std::thread m_thread;

               //
               // Block signals, so that broker doesn't pick up signals...
               //
               common::signal::thread::scope::Block m_signal_block;

               common::mockup::domain::Broker m_broker;

               // Links the global mockup-tm-queue to this process-receive-queue, hence, when
               // some message is sent to global-mockup-queue it will eventually get to this 'local-queue'
               common::mockup::ipc::Link tm_queue_link;

            };


            namespace mockup
            {

               namespace send
               {
                  template< typename M>
                  common::Uuid tm( M&& message)
                  {
                     common::log::internal::debug << "mockup::send::tm\n";

                     common::queue::blocking::Send send;
                     return send( common::mockup::ipc::transaction::manager::id(), std::forward< M>( message));
                  }

               } // send

               namespace ping
               {
                  bool tm()
                  {
                     common::log::internal::debug << "mockup::ping::tm\n";

                     common::mockup::ipc::Instance caller{ 501};

                     common::message::server::ping::Request request;
                     request.process = caller.process();
                     common::queue::blocking::Send send;
                     auto correlation = send( common::mockup::ipc::transaction::manager::id(), request);

                     common::message::server::ping::Reply reply;
                     common::queue::blocking::Reader receive{ caller.output()};
                     receive( reply);

                     return reply.correlation == correlation;
                  }
               } // ping

               common::message::transaction::begin::Reply begin( common::mockup::ipc::Instance& caller, std::chrono::microseconds timout = std::chrono::microseconds{ 0})
               {

                  common::log::internal::debug << "mockup::begin\n";

                  common::message::transaction::begin::Request request;
                  request.process = caller.process();

                  //
                  // We've got a 1s delay on the timeout (to get "better" semantics) , so we subtract 1s from 'now'
                  //
                  request.start = common::platform::clock_type::now() - std::chrono::seconds{ 1};
                  request.timeout = timout;

                  auto correlation = local::mockup::send::tm( request);


                  common::queue::blocking::Reader receive{ caller.output()};
                  common::message::transaction::begin::Reply reply;
                  receive( reply, correlation);

                  return reply;
               }

               namespace commit
               {
                  common::message::transaction::commit::Reply reply( common::mockup::ipc::Instance& caller, const common::Uuid& correlation)
                  {
                     common::log::internal::debug << "mockup::commit::reply - correlation: " << correlation << "\n";

                     common::queue::blocking::Reader receive{ caller.output()};
                     common::message::transaction::commit::Reply reply;
                     receive( reply, correlation);

                     return reply;

                  }

                  common::Uuid request( common::mockup::ipc::Instance& caller, const common::transaction::ID& trid)
                  {
                     common::message::transaction::commit::Request request;
                     request.process = caller.process();
                     request.trid = trid;

                     auto correlation = local::mockup::send::tm( request);
                     common::log::internal::debug << "mockup::commit::reqeust - trid: " << trid << ", correlation: " << correlation << "\n";

                     return correlation;
                  }

               } // commit

               common::message::transaction::rollback::Reply rollback( common::mockup::ipc::Instance& caller, const common::transaction::ID& trid)
               {
                  common::log::internal::debug << "mockup::rollback\n";

                  common::message::transaction::rollback::Request request;
                  request.process = caller.process();
                  request.trid = trid;

                  auto correlation = local::mockup::send::tm( request);

                  common::queue::blocking::Reader receive{ caller.output()};
                  common::message::transaction::rollback::Reply reply;
                  receive( reply, correlation);

                  return reply;
               }


               namespace error_state
               {
                  const common::transaction::ID& rm_fail()
                  {
                     static auto id = common::transaction::ID::create();
                     return id;
                  }

                  int get( const common::transaction::ID& trid)
                  {
                     static const std::map< common::transaction::ID, int> mapping{
                        {  rm_fail(), XAER_RMFAIL},
                     };

                     try
                     {
                        return mapping.at( trid);
                     }
                     catch( ...)
                     {
                     }
                     return XA_OK;
                  }

               } // error_state



               struct rm_proxy
               {

                  rm_proxy( common::platform::resource::id_type rm_id) : proxy{
                     current_pid++,
                     common::mockup::transform::Handler{
                        // prepare
                        [=]( common::message::transaction::resource::prepare::Request message)
                        {
                           common::log::internal::debug << "rm-proxy prepare::Request\n";

                           common::message::transaction::resource::prepare::Reply reply;
                           reply.correlation = message.correlation;
                           reply.trid = message.trid;
                           reply.resource = rm_id;
                           reply.process.pid = current_pid - 1;
                           reply.state = error_state::get( message.trid);

                           send::tm( reply);

                           return std::vector< common::ipc::message::Complete>{};
                        },
                        // commit
                        [=]( common::message::transaction::resource::commit::Request message)
                        {
                           common::log::internal::debug << "rm-proxy commit::Request\n";

                           common::message::transaction::resource::commit::Reply reply;
                           reply.correlation = message.correlation;
                           reply.trid = message.trid;
                           reply.resource = rm_id;
                           reply.process.pid = current_pid - 1;
                           reply.state = error_state::get( message.trid);

                           send::tm( reply);

                           return std::vector< common::ipc::message::Complete>{};
                        },
                        // rollback
                        [=]( common::message::transaction::resource::rollback::Request message)
                        {
                           common::log::internal::debug << "rm-proxy rollback::Request\n";

                           common::message::transaction::resource::rollback::Reply reply;
                           reply.correlation = message.correlation;
                           reply.trid = message.trid;
                           reply.resource = rm_id;
                           reply.process.pid = current_pid - 1;
                           reply.state = error_state::get( message.trid);

                           send::tm( reply);

                           return std::vector< common::ipc::message::Complete>{};
                        },
                     }
                  }
                  {

                     common::log::internal::debug << "rm-proxy resource connect\n";

                     common::message::transaction::resource::connect::Reply connect;
                     connect.resource = rm_id;
                     connect.state = XA_OK;
                     connect.process = proxy.process();

                     send::tm( connect);
                  }

                  common::mockup::ipc::Instance proxy;

                  static common::platform::pid_type current_pid;
               };
               common::platform::pid_type rm_proxy::current_pid = 10;




               struct State : ::casual::transaction::State
               {
                  using ::casual::transaction::State::State;

                  using instance_type = mockup::rm_proxy;


                  void add( state::resource::Proxy&& proxy)
                  {
                     for( std::size_t index = 0; index < proxy.concurency; ++index)
                     {
                        instance_type mockup{ proxy.id};

                        state::resource::Proxy::Instance instance;
                        instance.id = proxy.id;
                        instance.process = mockup.proxy.process();
                        instance.state( state::resource::Proxy::Instance::State::started);

                        proxy.instances.push_back( std::move( instance));

                        mockups.emplace( mockup.proxy.process().pid, std::move( mockup));
                     }
                     resources.push_back( std::move( proxy));
                  }

                  instance_type& get( common::platform::pid_type pid)
                  {
                     return mockups.at( pid);
                  }

                  std::map< common::platform::pid_type, instance_type> mockups;
               };

            } // mockup

            struct domain_0
            {
               domain_0() : state{ ":memory:"} {}

               mockup::State state;
            };

            struct domain_1 : domain_0
            {
               domain_1()
               {
                  {
                     state::resource::Proxy proxy;

                     proxy.id = 1;
                     proxy.key = "rm-mockup";
                     proxy.concurency = 2;
                     proxy.openinfo = "some open info 1";
                     proxy.closeinfo = "some close info 1";

                     state.add( std::move( proxy));
                  }

                  {
                     state::resource::Proxy proxy;

                     proxy.id = 2;
                     proxy.key = "rm-mockup";
                     proxy.concurency = 2;
                     proxy.openinfo = "some open info 2";
                     proxy.closeinfo = "some close info 2";

                     state.add( std::move( proxy));
                  }

                  common::range::sort( common::range::make( state.resources));
               }
            };

         } // <unnamed>
      } // local


      TEST( casual_transaction_manager, shutdown)
      {
         local::domain_0 domain;

         EXPECT_NO_THROW({
            local::Manager manager{ domain.state};
         });
      }


      TEST( casual_transaction_manager, resource_proxy_connect)
      {
         local::domain_1 domain;

         {
            local::Manager manager{ domain.state};

            //
            // Just to make sure we don't shutdown before mockup-resource-proxies has replied.
            //
            EXPECT_TRUE( local::mockup::ping::tm());
         }

         EXPECT_TRUE( ! domain.state.resources.empty()) << "resources: " << domain.state.resources.size();
         for( auto& proxy : domain.state.resources)
         {
            for( auto& instance : proxy.instances)
            {
               EXPECT_TRUE( instance.state() == state::resource::Proxy::Instance::State::idle);
            }
         }
      }

      TEST( casual_transaction_manager, begin_transaction)
      {
         common::mockup::ipc::Instance server1{ 500};

         local::domain_1 domain;

         common::message::transaction::begin::Reply reply;

         {
            local::Manager manager{ domain.state};

            reply = local::mockup::begin( server1);
         }

         auto trans = domain.state.log.select( reply.trid);
         ASSERT_TRUE( trans.size() == 1);
         EXPECT_TRUE( trans.at( 0).pid == server1.process().pid);
         EXPECT_TRUE( trans.at( 0).trid  == reply.trid);
         EXPECT_TRUE( trans.at( 0).state == Log::State::cBegin);
      }

      TEST( casual_transaction_manager, begin_commit_transaction__no_resources_involved__expect_XA_RDONLY)
      {
         common::mockup::ipc::Instance server1{ 500};

         local::domain_1 domain;

         common::transaction::ID trid;


         {
            local::Manager manager{ domain.state};

            // begin
            {
               auto reply = local::mockup::begin( server1);
               trid = reply.trid;
            }

            // commit
            {

               auto correlation = local::mockup::commit::request( server1, trid);

               auto reply = local::mockup::commit::reply( server1, correlation);

               //
               // We expect read-only optimization.
               //

               EXPECT_TRUE( reply.stage == common::message::transaction::commit::Reply::Stage::commit);
               EXPECT_TRUE( reply.trid == trid);
               EXPECT_TRUE( reply.state == XA_RDONLY);
            }
         }

         auto trans = domain.state.log.select( trid);
         EXPECT_TRUE( trans.empty());
      }


      TEST( casual_transaction_manager, commit_non_existent_transaction__expect_XAER_NOTA)
      {
         common::mockup::ipc::Instance server1{ 500};

         local::domain_1 domain;

         common::transaction::ID trid = common::transaction::ID::create( server1.process());

         {
            local::Manager manager{ domain.state};

            // commit
            {

               auto correlation = local::mockup::commit::request( server1, trid);

               auto reply = local::mockup::commit::reply( server1, correlation);

               EXPECT_TRUE( reply.stage == common::message::transaction::commit::Reply::Stage::error);
               EXPECT_TRUE( reply.trid == trid);
               EXPECT_TRUE( reply.state == XAER_NOTA);
            }
         }

         auto trans = domain.state.log.select( trid);
         EXPECT_TRUE( trans.empty());
      }

      TEST( casual_transaction_manager, rollback_non_existent_transaction__expect_XAER_NOTA)
      {
         common::mockup::ipc::Instance server1{ 500};

         local::domain_1 domain;

         common::transaction::ID trid = common::transaction::ID::create( server1.process());

         {
            local::Manager manager{ domain.state};

            // rollback
            {

               auto reply = local::mockup::rollback( server1, trid);

               EXPECT_TRUE( reply.stage == common::message::transaction::rollback::Reply::Stage::error);
               EXPECT_TRUE( reply.trid == trid);
               EXPECT_TRUE( reply.state == XAER_NOTA);
            }
         }

         auto trans = domain.state.log.select( trid);
         EXPECT_TRUE( trans.empty());
      }


      TEST( casual_transaction_manager, begin_commit_transaction__1_resources_involved__expect_one_phase_commit_optimization)
      {
         common::mockup::ipc::Instance caller{ 500};

         local::domain_1 domain;

         common::transaction::ID trid;


         {
            local::Manager manager{ domain.state};

            // begin
            {
               auto reply = local::mockup::begin( caller);
               trid = reply.trid;
            }

            // involved
            {
               common::message::transaction::resource::Involved message;
               message.trid = trid;
               message.process = caller.process();
               message.resources = { domain.state.resources.at( 0).id};

               local::mockup::send::tm( message);
            }

            // commit
            {

               auto correlation = local::mockup::commit::request( caller, trid);

               //
               // We expect committed
               //
               {
                  auto reply = local::mockup::commit::reply( caller, correlation);

                  EXPECT_TRUE( reply.stage == common::message::transaction::commit::Reply::Stage::commit);
                  EXPECT_TRUE( reply.trid == trid);
                  EXPECT_TRUE( reply.state == XA_OK);
               }
            }
         }

         auto trans = domain.state.log.select( trid);
         EXPECT_TRUE( trans.empty());
      }


      TEST( casual_transaction_manager, begin_rollback_transaction__1_resources_involved__expect_XA_OK)
      {
         common::mockup::ipc::Instance caller{ 500};

         local::domain_1 domain;

         common::transaction::ID trid;


         {
            local::Manager manager{ domain.state};

            // begin
            {
               auto reply = local::mockup::begin( caller);
               trid = reply.trid;
            }

            // involved
            {
               common::message::transaction::resource::Involved message;
               message.trid = trid;
               message.process = caller.process();
               message.resources = { domain.state.resources.at( 0).id};

               local::mockup::send::tm( message);
            }

            // rollback
            {

               auto reply = local::mockup::rollback( caller, trid);

               EXPECT_TRUE( reply.stage == common::message::transaction::rollback::Reply::Stage::rollback);
               EXPECT_TRUE( reply.trid == trid);
               EXPECT_TRUE( reply.state == XA_OK);
            }
         }

         auto trans = domain.state.log.select( trid);
         EXPECT_TRUE( trans.empty());
      }


      TEST( casual_transaction_manager, begin_rollback_transaction__2_resources_involved__expect_XA_OK)
      {
         common::mockup::ipc::Instance caller{ 500};

         local::domain_1 domain;

         common::transaction::ID trid;


         {
            local::Manager manager{ domain.state};

            // begin
            {
               auto reply = local::mockup::begin( caller);
               trid = reply.trid;
            }

            // involved
            {
               common::message::transaction::resource::Involved message;
               message.trid = trid;
               message.process = caller.process();
               message.resources = { domain.state.resources.at( 0).id, domain.state.resources.at( 1).id};

               local::mockup::send::tm( message);
            }

            // rollback
            {

               auto reply = local::mockup::rollback( caller, trid);

               EXPECT_TRUE( reply.stage == common::message::transaction::rollback::Reply::Stage::rollback);
               EXPECT_TRUE( reply.trid == trid);
               EXPECT_TRUE( reply.state == XA_OK);
            }
         }

         auto trans = domain.state.log.select( trid);
         EXPECT_TRUE( trans.empty());
      }


      TEST( casual_transaction_manager, begin_commit_transaction__2_resources_involved__expect_two_phase_commit)
      {
         common::mockup::ipc::Instance caller{ 500};

         local::domain_1 domain;

         common::transaction::ID trid;


         {
            local::Manager manager{ domain.state};

            // begin
            {
               auto reply = local::mockup::begin( caller);
               trid = reply.trid;
            }

            // involved
            {
               common::message::transaction::resource::Involved message;
               message.trid = trid;
               message.process = caller.process();
               message.resources = { domain.state.resources.at( 0).id, domain.state.resources.at( 1).id};

               local::mockup::send::tm( message);
            }

            // commit
            {

               auto correlation = local::mockup::commit::request( caller, trid);

               //
               // We expect prepared
               //
               {
                  auto reply = local::mockup::commit::reply( caller, correlation);

                  EXPECT_TRUE( reply.stage == common::message::transaction::commit::Reply::Stage::prepare);
                  EXPECT_TRUE( reply.trid == trid);
                  EXPECT_TRUE( reply.state == XA_OK);
               }

               //
               // We expect committed
               //
               {
                  auto reply = local::mockup::commit::reply( caller, correlation);

                  EXPECT_TRUE( reply.stage == common::message::transaction::commit::Reply::Stage::commit);
                  EXPECT_TRUE( reply.trid == trid);
                  EXPECT_TRUE( reply.state == XA_OK);
               }
            }
         }

         auto trans = domain.state.log.select( trid);
         EXPECT_TRUE( trans.empty());
      }

      TEST( casual_transaction_manager, begin_transaction__1_resource_involved__owner_dies__expect_rollback)
      {
         common::mockup::ipc::Instance caller{ 500};

         local::domain_1 domain;

         common::transaction::ID trid;


         {
            local::Manager manager{ domain.state};

            // begin
            {
               auto reply = local::mockup::begin( caller);
               trid = reply.trid;
            }

            // involved
            {
               common::message::transaction::resource::Involved message;
               message.trid = trid;
               message.process = caller.process();
               message.resources = { domain.state.resources.at( 0).id};

               local::mockup::send::tm( message);
            }

            // caller dies
            {
               common::message::dead::process::Event event;
               event.death.pid = caller.process().pid;
               event.death.reason = common::process::lifetime::Exit::Reason::core;

               local::mockup::send::tm( event);
            }

            local::mockup::ping::tm();

            // 5ms should be more than enough for the TM to finish the rollback.
            common::process::sleep( std::chrono::milliseconds{ 5});
         }

         auto trans = domain.state.log.select( trid);
         EXPECT_TRUE( trans.empty());
      }

   } // transaction

} // casual
