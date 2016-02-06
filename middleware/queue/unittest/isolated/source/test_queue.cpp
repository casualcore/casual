//!
//! test_queue.cpp
//!
//! Created on: Aug 15, 2015
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "queue/group/group.h"
#include "queue/common/environment.h"
#include "queue/api/rm/queue.h"
#include "queue/broker/admin/queuevo.h"
#include "queue/rm/switch.h"

#include "common/mockup/domain.h"
#include "common/mockup/process.h"

#include "common/transaction/context.h"
#include "common/transaction/resource.h"

#include "sf/xatmi_call.h"


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
                  : m_filename{ configuration_file( configuration)},
                    m_process{ "./bin/casual-queue-broker", {
                        "-c", m_filename,
                        "-g", "./bin/casual-queue-group",
                      }}
               {

                  //
                  // We need to re-initialize the casual-queue ipc-queue, since it's only done once otherwise
                  //
                  queue::environment::broker::queue::initialize();
               }

               static std::string configuration_file(  const std::string& configuration)
               {
                  auto filename = common::file::name::unique( common::directory::temporary() + '/', ".yaml");
                  std::ofstream file{ filename};
                  file << configuration;
                  return filename;
               }

            private:
               common::file::scoped::Path m_filename;
               common::mockup::Process m_process;
            };

            struct Domain
            {
               Domain( const std::string& configuration)
                : broker{ create_resources()}, queue_broker{ configuration}
                {
                   common::transaction::Resource resource{ "casual-queue-rm", &casual_queue_xa_switch_dynamic};
                   common::transaction::Context::instance().set( { resource});

                }

               common::mockup::domain::Broker broker;
               common::mockup::domain::transaction::Manager tm;

               Broker queue_broker;

            private:
               static common::mockup::domain::broker::transaction::client::Connect create_resources()
               {
                  common::message::transaction::client::connect::Reply result;

                  result.directive = common::message::transaction::client::connect::Reply::Directive::start;
                  decltype( result.resources)::value_type resource;

                  resource.instances = 1;
                  resource.id = 10;
                  resource.key = "casual-queue-rm";

                  result.resources.push_back( std::move( resource));


                  return { result};
               }
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
  name: test-queue
  
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
         local::Domain domain{ local::configuration()};


         auto state = local::call::state();

         EXPECT_TRUE( state.groups.size() == 2);
         EXPECT_TRUE( state.queues.size() == 3 * 2 * 2 + 2);
      }

      TEST( casual_queue, enqueue_1_message___expect_1_message_in_queue)
      {
         local::Domain domain{ local::configuration()};


         const std::string payload{ "some message"};
         queue::Message message;
         message.payload.type.type = common::buffer::type::binary().name;
         message.payload.type.subtype = common::buffer::type::binary().subname;
         message.payload.data.assign( std::begin( payload), std::end( payload));

         queue::rm::enqueue( "queueA1", message);
         auto messages = local::call::messages( "queueA1");

         EXPECT_TRUE( messages.size() == 1);
      }

      TEST( casual_queue, enqueue_5_message___expect_5_message_in_queue)
      {
         local::Domain domain{ local::configuration()};

         auto count = 5;
         while( count-- > 0)
         {
            const std::string payload{ "some message"};
            queue::Message message;
            message.payload.data.assign( std::begin( payload), std::end( payload));

            queue::rm::enqueue( "queueA1", message);
         }

         auto messages = local::call::messages( "queueA1");

         EXPECT_TRUE( messages.size() == 5);
      }

   } // queue
} // casual
