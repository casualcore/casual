//!
//! casual
//!

#include "queue/manager/admin/server.h"
#include "queue/manager/admin/services.h"

#include "queue/manager/manager.h"
#include "queue/manager/handle.h"
#include "queue/common/transform.h"


#include "sf/service/protocol.h"

#include "xatmi.h"

namespace casual
{
   namespace queue
   {
      namespace manager
      {
         namespace admin
         {

            namespace local
            {
               namespace
               {
                  common::service::invoke::Result state( common::service::invoke::Parameter&& parameter, manager::State& state)
                  {
                     auto protocol = sf::service::protocol::deduce( std::move( parameter));

                     auto result = sf::service::user( protocol, &admin::state, state);

                     protocol << CASUAL_MAKE_NVP( result);
                     return protocol.finalize();
                  }

                  namespace list
                  {
                     common::service::invoke::Result messages( common::service::invoke::Parameter&& parameter, manager::State& state)
                     {
                        auto protocol = sf::service::protocol::deduce( std::move( parameter));

                        std::string queue;
                        protocol >> CASUAL_MAKE_NVP( queue);

                        auto result = sf::service::user( protocol, &admin::list_messages, state, queue);

                        protocol << CASUAL_MAKE_NVP( result);
                        return protocol.finalize();
                     }

                  } // list

                  common::service::invoke::Result restore( common::service::invoke::Parameter&& parameter, manager::State& state)
                  {
                     auto protocol = sf::service::protocol::deduce( std::move( parameter));

                     std::string queue;
                     protocol >> CASUAL_MAKE_NVP( queue);

                     auto result = sf::service::user( protocol, &admin::restore, state, queue);

                     protocol << CASUAL_MAKE_NVP( result);
                     return protocol.finalize();
                  }

               } // <unnamed>
            } // local


            admin::State state( manager::State& state)
            {
               admin::State result;

               result.queues = transform::queues( manager::queues( state));
               result.groups = transform::groups( state);

               return result;
            }

            std::vector< Message> list_messages( manager::State& state, const std::string& queue)
            {
               return transform::messages( manager::messages( state, queue));
            }



            std::vector< Affected> restore( manager::State& state, const std::string& queuename)
            {
               std::vector< Affected> result;

               auto found = common::range::find( state.queues, queuename);

               if( found && ! found->second.empty() && found->second.front().order == 0)
               {
                  auto&& queue = found->second.front();

                  common::message::queue::restore::Request request;
                  request.process = common::process::handle();
                  request.queues.push_back( queue.queue);

                  auto reply = manager::ipc::device().call( queue.process.queue, request);

                  if( ! reply.affected.empty())
                  {
                     auto& restored = reply.affected.front();
                     Affected affected;
                     affected.queue.name = queuename;
                     affected.queue.id = restored.queue;
                     affected.restored = restored.restored;

                     result.push_back( std::move( affected));
                  }
               }
               return result;
            }


            common::server::Arguments services( manager::State& state)
            {
               return { {
                     { service::name::state(),
                        std::bind( &local::state, std::placeholders::_1, std::ref( state)),
                        common::service::transaction::Type::none,
                        common::service::category::admin()
                     },
                     { service::name::list_messages(),
                        std::bind( &local::list::messages, std::placeholders::_1, std::ref( state)),
                        common::service::transaction::Type::none,
                        common::service::category::admin()
                     },
                     { service::name::restore(),
                        std::bind( &local::restore, std::placeholders::_1, std::ref( state)),
                        common::service::transaction::Type::none,
                        common::service::category::admin()
                     }
               }};
            }
         } // admin
      } // manager
   } // queue


} // casual
