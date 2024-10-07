#include "casual/argument.h"
#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/exception/guard.h"
#include "common/message/dispatch.h"
#include "common/message/dispatch/handle.h"
#include "common/server/argument.h"
#include "common/server/handle/call.h"
#include "queue/api/queue.h"

namespace casual
{
   namespace queue::example
   {
      namespace local
      {
         namespace
         {
            struct
            {
               std::string queue;
            } settings;

            namespace service
            {
               auto enqueue()
               {
                  return []( common::service::invoke::Parameter&& parameter)
                  {
                     queue::Message message;
                     message.payload.data = std::move(parameter.payload.data);
                     message.payload.type = parameter.payload.type;
                     queue::enqueue(settings.queue, message);
                     return common::service::invoke::Result{ nullptr };
                  };
               }

               auto dequeue()
               {
                  return []( common::service::invoke::Parameter&& parameter)
                  {
                     if (auto message = queue::dequeue( settings.queue); ! message.empty())
                     {
                        common::buffer::Payload payload { message.front().payload.type };
                        payload.data = std::move( message.front().payload.data);
                        return common::service::invoke::Result{ std::move(payload) };
                     }
                     return common::service::invoke::Result{ nullptr };
                  };
               }
            }

            common::server::Arguments services()
            {
               return {{
                  {
                     "casual/example/enqueue",
                     service::enqueue(),
                     common::service::transaction::Type::automatic,
                     common::service::visibility::Type::discoverable,
                     "example"
                  },
                  {
                     "casual/example/dequeue",
                     service::dequeue(),
                     common::service::transaction::Type::automatic,
                     common::service::visibility::Type::discoverable,
                     "example"
                  }
               }};
            }

            void main( int argc, const char** argv)
            {
               argument::parse(
                  R"(Example server)", {
                  argument::Option{ std::tie( settings.queue), {"--queue"}, "queue that casual/example/{enqueue,dequeue} should use"},
               }, argc, argv);

               common::log::line( common::log::category::information, "queue: ", settings.queue);

               common::communication::instance::connect();

               auto handler = common::message::dispatch::handler(
                  common::communication::ipc::inbound::device(),
                  common::message::dispatch::handle::defaults(),
                  common::server::handle::Call{services()}
               );

               common::message::dispatch::pump( handler, common::communication::ipc::inbound::device());
            }

         }
      }
   }
}

int main( int argc, const char** argv)
{
   return casual::common::exception::main::log::guard([=]()
   {
      casual::queue::example::local::main( argc, argv);
   });
}
