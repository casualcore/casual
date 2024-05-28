#include "common/argument.h"
#include "common/algorithm.h"
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
               std::vector<std::string> queues;
            } settings;

            namespace service
            {
               auto enqueue( const std::string& queue)
               {
                  return [queue]( common::service::invoke::Parameter&& parameter)
                  {
                     queue::Message message;
                     message.payload.data = std::move(parameter.payload.data);
                     message.payload.type = parameter.payload.type;
                     queue::enqueue(queue, message);
                     return common::service::invoke::Result{ nullptr };
                  };
               }

               auto dequeue( const std::string& queue)
               {
                  return [queue]( common::service::invoke::Parameter&& parameter)
                  {
                     if (auto message = queue::dequeue( queue); ! message.empty())
                     {
                        common::buffer::Payload payload { message.front().payload.type };
                        payload.data = std::move( message.front().payload.data);
                        return common::service::invoke::Result{ std::move(payload) };
                     }
                     return common::service::invoke::Result{ nullptr };
                  };
               }
            }

            common::server::Arguments services( std::vector<std::string> queues)
            {
               auto to_services = [](const auto& queue) -> std::vector<common::server::Service>
               {
                  return {
                     {
                        common::string::compose( "casual/example/enqueue/", queue),
                        service::enqueue( queue),
                        common::service::transaction::Type::automatic,
                        common::service::visibility::Type::discoverable,
                        "example"
                     },
                     {
                        common::string::compose( "casual/example/dequeue/", queue),
                        service::dequeue( queue),
                        common::service::transaction::Type::automatic,
                        common::service::visibility::Type::discoverable,
                        "example"
                     },
                  };
               };

               auto services = common::algorithm::accumulate(
                  common::algorithm::transform( queues, to_services),
                  std::vector<common::server::Service>{},
                  [](auto&& a, auto&& b) {
                     return common::algorithm::append( b, a);
                  });

               return {{ services }};
            }

            void main( int argc, char **argv)
            {
               common::argument::Parse{
                  R"(Example server)",
                  common::argument::Option{ std::tie( settings.queues), {"--queues"}, "queues that casual/example/{enqueue,dequeue} should use"},
               }(argc, argv);

               common::log::line( common::log::category::information, "queues: ", settings.queues);
               common::communication::instance::connect();

               auto handler = common::message::dispatch::handler(
                  common::communication::ipc::inbound::device(),
                  common::message::dispatch::handle::defaults(),
                  common::server::handle::Call{services(settings.queues)}
               );

               common::message::dispatch::pump( handler, common::communication::ipc::inbound::device());
            }

         }
      }
   }
}

int main( int argc, char **argv)
{
   return casual::common::exception::main::log::guard([=]()
   {
      casual::queue::example::local::main( argc, argv);
   });
}
