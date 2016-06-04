//!
//! casual
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "queue/group/group.h"
#include "queue/common/environment.h"
#include "queue/api/rm/queue.h"
#include "queue/broker/admin/queuevo.h"
#include "queue/rm/switch.h"

#include "common/mockup/domain.h"
#include "common/mockup/process.h"
#include "common/mockup/file.h"

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
               : manager{ handle_resource_configuration{}}, queue_broker{ configuration}
               {
                  common::transaction::Resource resource{ "casual-queue-rm", &casual_queue_xa_switch_dynamic};
                  common::transaction::Context::instance().set( { resource});

                  //
                  // We make sure queue-broker is up'n running, we send ping
                  //
                  common::process::ping( queue_broker.process().queue);
               }

               common::mockup::domain::Manager manager;
               common::mockup::domain::Broker broker;
               common::mockup::domain::transaction::Manager tm;

               Broker queue_broker;

            private:

               struct handle_resource_configuration
               {
                  void operator () ( common::message::domain::configuration::transaction::resource::Request& request) const
                  {
                     auto reply = common::message::reverse::type( request);

                     reply.resources.emplace_back(
                           []( common::message::domain::configuration::transaction::Resource& m)
                           {
                              m.id = 10;
                              m.key = "casual-queue-rm";
                              m.instances = 1;
                              m.openinfo = "id:10";
                           });

                     common::mockup::ipc::eventually::send( request.process.queue, reply);
                  }
               };
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
         CASUAL_UNITTEST_TRACE();

         local::Domain domain{ local::configuration()};


         auto state = local::call::state();

         EXPECT_TRUE( state.groups.size() == 2);
         EXPECT_TRUE( state.queues.size() == 3 * 2 * 2 + 2);
      }

      TEST( casual_queue, enqueue_1_message___expect_1_message_in_queue)
      {
         CASUAL_UNITTEST_TRACE();

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
         CASUAL_UNITTEST_TRACE();

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
