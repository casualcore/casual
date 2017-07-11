//!
//! casual
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "queue/group/group.h"
#include "queue/common/environment.h"
#include "queue/common/transform.h"
#include "queue/api/queue.h"
#include "queue/manager/admin/queuevo.h"
#include "queue/manager/admin/services.h"

#include "common/process.h"
#include "common/message/gateway.h"
#include "common/message/domain.h"
#include "common/mockup/domain.h"
#include "common/mockup/process.h"
#include "common/mockup/file.h"

#include "common/transaction/context.h"
#include "common/transaction/resource.h"

#include "sf/service/protocol/call.h"
#include "sf/namevaluepair.h"
#include "sf/log.h"


#include <fstream>

namespace casual
{

   namespace queue
   {
      namespace local
      {
         namespace
         {

            using config_domain = common::message::domain::configuration::Domain;


            struct Manager
            {

               Manager()
                    : m_process{ "./bin/casual-queue-manager", {
                        "-g", "./bin/casual-queue-group",
                      }}
               {

                  //
                  // Make sure we're up'n running before we let unittest-stuff interact with us...
                  //
                  common::process::instance::fetch::handle( common::process::instance::identity::queue::manager());
               }

               common::process::Handle process() const { return m_process.handle();}

            private:
               common::mockup::Process m_process;
            };

            struct Domain
            {
               Domain( config_domain configuration)
               : manager{ std::move( configuration)}
               {
               }

               common::mockup::domain::Manager manager;
               common::mockup::domain::service::Manager service;
               common::mockup::domain::transaction::Manager tm;

               Manager queue_manager;

            };

            namespace call
            {
               manager::admin::State state()
               {
                  sf::service::protocol::binary::Call call;
                  auto reply = call( manager::admin::service::name::state());

                  manager::admin::State result;
                  reply >> CASUAL_MAKE_NVP( result);

                  return result;
               }

               std::vector< manager::admin::Message> messages( const std::string& queue)
               {
                  sf::service::protocol::binary::Call call;
                  call << CASUAL_MAKE_NVP( queue);
                  auto reply = call( manager::admin::service::name::list_messages());

                  std::vector< manager::admin::Message> result;
                  reply >> CASUAL_MAKE_NVP( result);

                  return result;
               }
            } // call

            config_domain configuration()
            {
               config_domain domain;

               domain.queue.groups.resize( 2);
               {
                  auto& group = domain.queue.groups.at( 0);
                  group.name = "group_A";
                  group.queuebase = ":memory:";

                  using queue_t = common::message::domain::configuration::queue::Queue;

                  group.queues = {
                        { []( queue_t& q){
                           q.name = "queueA1";
                           q.retries = 3;
                        }},
                        { []( queue_t& q){
                           q.name = "queueA2";
                           q.retries = 3;
                        }},
                        { []( queue_t& q){
                           q.name = "queueA3";
                           q.retries = 3;
                        }}
                  };
               }
               {
                  auto& group = domain.queue.groups.at( 1);
                  group.name = "group_B";
                  group.queuebase = ":memory:";

                  using queue_t = common::message::domain::configuration::queue::Queue;

                  group.queues = {
                        { []( queue_t& q){
                           q.name = "queueB1";
                           q.retries = 3;
                        }},
                        { []( queue_t& q){
                           q.name = "queueB2";
                           q.retries = 3;
                        }},
                        { []( queue_t& q){
                           q.name = "queueB3";
                           q.retries = 3;
                        }}
                  };
               }

               return domain;
            }

         } // <unnamed>

      } // local




      TEST( casual_queue, manager_startup)
      {
         common::unittest::Trace trace;

         local::Domain domain{ local::configuration()};


         auto state = local::call::state();

         EXPECT_TRUE( state.groups.size() == 2);
         EXPECT_TRUE( state.queues.size() == 3 * 2 * 2 + 2);
      }

      TEST( casual_queue, enqueue_1_message___expect_1_message_in_queue)
      {
         common::unittest::Trace trace;

         local::Domain domain{ local::configuration()};


         const std::string payload{ "some message"};
         queue::Message message;
         message.payload.type = common::buffer::type::binary();
         message.payload.data.assign( std::begin( payload), std::end( payload));

         queue::enqueue( "queueA1", message);
         auto messages = local::call::messages( "queueA1");

         EXPECT_TRUE( messages.size() == 1);
      }

      TEST( casual_queue, enqueue_5_message___expect_5_message_in_queue)
      {
         common::unittest::Trace trace;

         local::Domain domain{ local::configuration()};

         auto count = 5;
         while( count-- > 0)
         {
            const std::string payload{ "some message"};
            queue::Message message;
            message.payload.data.assign( std::begin( payload), std::end( payload));

            queue::enqueue( "queueA1", message);
         }

         auto messages = local::call::messages( "queueA1");

         EXPECT_TRUE( messages.size() == 5);
      }
      namespace local
      {
         namespace
         {
            bool compare( const common::platform::time::point::type& lhs, const common::platform::time::point::type& rhs)
            {
               return std::chrono::time_point_cast< std::chrono::microseconds>( lhs)
                     == std::chrono::time_point_cast< std::chrono::microseconds>( rhs);
            }
         } // <unnamed>
      } // local

      TEST( casual_queue, enqueue_1_message__peek__information__expect_1_peeked)
      {
         common::unittest::Trace trace;

         local::Domain domain{ local::configuration()};

         auto now = common::platform::time::clock::type::now();

         const std::string payload{ "some message"};
         queue::Message message;
         {
            message.attributes.available = now;
            message.attributes.properties = "poop";
            message.attributes.reply = "queueA2";
            message.payload.type = common::buffer::type::binary();
            message.payload.data.assign( std::begin( payload), std::end( payload));
         }

         auto id = queue::enqueue( "queueA1", message);

         auto information = queue::peek::information( "queueA1");

         ASSERT_TRUE( information.size() == 1);
         auto& info = information.at( 0);
         EXPECT_TRUE( info.id == id);
         EXPECT_TRUE( local::compare( info.attributes.available, now));
         EXPECT_TRUE( info.attributes.properties == "poop") << "info: " << CASUAL_MAKE_NVP( info);
         EXPECT_TRUE( info.attributes.reply == "queueA2");
         EXPECT_TRUE( info.payload.type == common::buffer::type::binary());
         EXPECT_TRUE( info.payload.size == payload.size());

         // message should still be there
         {
            auto messages = queue::dequeue( "queueA1");
            EXPECT_TRUE( ! messages.empty());
            auto& message = messages.at( 0);
            EXPECT_TRUE( message.id == id);
            EXPECT_TRUE( common::range::equal( message.payload.data, payload));
         }
      }

      TEST( casual_queue, enqueue_1_message__peek_message__expect_1_peeked)
      {
         common::unittest::Trace trace;

         local::Domain domain{ local::configuration()};

         auto now = common::platform::time::clock::type::now();

         const std::string payload{ "some message"};

         auto enqueue = [&](){
            queue::Message message;
            {
               message.attributes.available = now;
               message.attributes.properties = "poop";
               message.attributes.reply = "queueA2";
               message.payload.type = common::buffer::type::binary();
               message.payload.data.assign( std::begin( payload), std::end( payload));
            }

            return queue::enqueue( "queueA1", message);
         };

         auto id = enqueue();

         auto messages = queue::peek::messages( "queueA1", { id});

         ASSERT_TRUE( messages.size() == 1);
         auto& message = messages.at( 0);
         EXPECT_TRUE( message.id == id) << "message: " << CASUAL_MAKE_NVP( message);
         EXPECT_TRUE( local::compare( message.attributes.available, now));
         EXPECT_TRUE( message.attributes.properties == "poop");
         EXPECT_TRUE( message.attributes.reply == "queueA2");
         EXPECT_TRUE( message.payload.type == common::buffer::type::binary());
         EXPECT_TRUE( common::range::equal( message.payload.data, payload));

         // message should still be there
         {
            auto messages = queue::dequeue( "queueA1");
            EXPECT_TRUE( ! messages.empty());
            auto& message = messages.at( 0);
            EXPECT_TRUE( message.id == id);
            EXPECT_TRUE( common::range::equal( message.payload.data, payload));
         }
      }

      TEST( casual_queue, peek_remote_message___expect_throw)
      {
         common::unittest::Trace trace;

         local::Domain domain{ local::configuration()};

         // make sure casual-manager-queue knows about a "remote queue"
         {
            common::message::gateway::domain::Advertise remote;

            remote.process.pid = 666;
            remote.process.queue = 777;

            remote.queues.push_back( { "remote-queue"});

            common::communication::ipc::blocking::send( domain.queue_manager.process().queue, remote);
         }

         EXPECT_THROW({
            queue::peek::information( "remote-queue");
         }, common::exception::invalid::Argument);
      }

   } // queue
} // casual
