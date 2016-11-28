//!
//! casual
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "queue/group/group.h"
#include "queue/common/environment.h"
#include "queue/common/transform.h"
#include "queue/api/queue.h"
#include "queue/broker/admin/queuevo.h"

#include "common/message/gateway.h"
#include "common/mockup/domain.h"
#include "common/mockup/process.h"
#include "common/mockup/file.h"

#include "common/transaction/context.h"
#include "common/transaction/resource.h"

#include "sf/xatmi_call.h"
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

            struct Broker
            {

               Broker( const std::string& configuration)
                  : m_filename{ common::mockup::file::temporary( ".yaml", configuration)},
                    m_process{ "./bin/casual-queue-broker", {
                        "-c", m_filename,
                        "-g", "./bin/casual-queue-group",
                      }}
               {


               }

               common::process::Handle process() const { return m_process.handle();}

            private:
               common::file::scoped::Path m_filename;
               common::mockup::Process m_process;
            };

            struct Domain
            {
               Domain( const std::string& configuration)
               : queue_broker{ configuration}
               {

                  //
                  // We make sure queue-broker is up'n running, we send ping
                  //
                  common::process::ping( queue_broker.process().queue);
               }

               common::mockup::domain::Manager manager;
               common::mockup::domain::Broker broker;
               common::mockup::domain::transaction::Manager tm;

               Broker queue_broker;

            };

            namespace call
            {
               broker::admin::State state()
               {
                  sf::xatmi::service::binary::Sync service( ".casual.queue.list.queues");

                  auto reply = service();

                  broker::admin::State serviceReply;

                  reply >> CASUAL_MAKE_NVP( serviceReply);

                  return serviceReply;
               }

               std::vector< broker::admin::Message> messages( const std::string& queue)
               {
                  sf::xatmi::service::binary::Sync service( ".casual.queue.list.messages");
                  service << CASUAL_MAKE_NVP( queue);

                  auto reply = service();

                  std::vector< broker::admin::Message> serviceReply;

                  reply >> CASUAL_MAKE_NVP( serviceReply);

                  return serviceReply;
               }
            } // call

            std::string configuration()
            {
               return R"(

domain:
  queue:
  
     default:  
       queue:
         retries: 3
      
     groups:
       - name: group_A
         queuebase: ":memory:"
         
         queues:
           - name: queueA1
           - name: queueA2
           - name: queueA3
       
       - name: group_B
         queuebase: ":memory:"
         
         queues:
           - name: queueB1
           - name: queueB2
           - name: queueB3
)";
            }

         } // <unnamed>

      } // local




      TEST( casual_queue, broker_startup)
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
            bool compare( const common::platform::time_point& lhs, const common::platform::time_point& rhs)
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

         auto now = common::platform::clock_type::now();

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

         auto now = common::platform::clock_type::now();

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

         // make sure casual-broker-queue knows about a "remote queue"
         {
            common::message::gateway::domain::Advertise remote;

            remote.process.pid = 666;
            remote.process.queue = 777;

            remote.queues.push_back( { "remote-queue"});

            common::communication::ipc::blocking::send( domain.queue_broker.process().queue, remote);
         }

         EXPECT_THROW({
            queue::peek::information( "remote-queue");
         }, common::exception::invalid::Argument);
      }

   } // queue
} // casual
