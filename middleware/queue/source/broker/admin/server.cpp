//!
//! casual
//!

#include "queue/broker/admin/server.h"

#include "queue/broker/broker.h"
#include "queue/broker/handle.h"
#include "queue/common/transform.h"


#include "sf/server.h"


namespace casual
{
   namespace queue
   {
      namespace local
      {
         namespace
         {
            sf::server::type server;


            namespace service
            {
               namespace name
               {
                  std::string state() { return { ".casual/queue/state"}; }
                  std::string restore() { return { ".casual/queue/restore"};}

               } // name

            } // service


         } // <unnamed>
      } // local

      extern "C"
      {
         int tpsvrinit( int argc, char **argv)
         {
            try
            {
               local::server = casual::sf::server::create( argc, argv);
            }
            catch( ...)
            {
               // TODO
               return -1;
            }

            return 0;
         }

         void tpsvrdone()
         {
            //
            // delete the implementation an server implementation
            //
            casual::sf::server::sink( local::server);
         }
      }



      namespace broker
      {
         namespace admin
         {

            namespace service
            {
               extern "C"
               {

                  void state( TPSVCINFO *serviceInfo, broker::State& state)
                  {
                     casual::sf::service::reply::State reply;

                     try
                     {
                        auto service_io = local::server->createService( serviceInfo);


                        auto serviceReturn = service_io.call( &admin::state, state);

                        service_io << CASUAL_MAKE_NVP( serviceReturn);

                        reply = service_io.finalize();
                     }
                     catch( ...)
                     {
                        local::server->handleException( serviceInfo, reply);
                     }

                     tpreturn(
                        reply.value,
                        reply.code,
                        reply.data,
                        reply.size,
                        reply.flags);
                  }


                  void list_messages( TPSVCINFO *serviceInfo, broker::State& state)
                  {
                     casual::sf::service::reply::State reply;

                     try
                     {
                        auto service_io = local::server->createService( serviceInfo);

                        std::string queue;
                        service_io >> CASUAL_MAKE_NVP( queue);

                        auto serviceReturn = service_io.call( &admin::list_messages, state, queue);

                        service_io << CASUAL_MAKE_NVP( serviceReturn);

                        reply = service_io.finalize();
                     }
                     catch( ...)
                     {
                        local::server->handleException( serviceInfo, reply);
                     }

                     tpreturn(
                        reply.value,
                        reply.code,
                        reply.data,
                        reply.size,
                        reply.flags);
                  }
               }

               void restore( TPSVCINFO *serviceInfo, broker::State& state)
               {
                  casual::sf::service::reply::State reply;

                  try
                  {
                     auto service_io = local::server->createService( serviceInfo);

                     std::string queue;
                     service_io >> CASUAL_MAKE_NVP( queue);

                     auto serviceReturn = service_io.call( &admin::restore, state, queue);

                     service_io << CASUAL_MAKE_NVP( serviceReturn);

                     reply = service_io.finalize();
                  }
                  catch( ...)
                  {
                     local::server->handleException( serviceInfo, reply);
                  }

                  tpreturn(
                     reply.value,
                     reply.code,
                     reply.data,
                     reply.size,
                     reply.flags);
               }
            } // service


            admin::State state( broker::State& state)
            {
               admin::State result;

               result.queues = transform::queues( broker::queues( state));
               result.groups = transform::groups( state);

               return result;
            }

            std::vector< Message> list_messages( broker::State& state, const std::string& queue)
            {
               return transform::messages( broker::messages( state, queue));
            }



            std::vector< Affected> restore( broker::State& state, const std::string& queuename)
            {
               std::vector< Affected> result;

               auto found = common::range::find( state.queues, queuename);

               if( found && ! found->second.empty() && found->second.front().order == 0)
               {
                  auto&& queue = found->second.front();

                  common::message::queue::restore::Request request;
                  request.process = common::process::handle();
                  request.queues.push_back( queue.queue);

                  auto reply = broker::ipc::device().call( queue.process.queue, request);

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


            common::server::Arguments services( broker::State& state)
            {
               common::server::Arguments result{ { common::process::path()}};

               result.services.emplace_back( local::service::name::state(),
                     std::bind( &service::state, std::placeholders::_1, std::ref( state)),
                     common::server::Service::Type::cCasualAdmin,
                     common::service::transaction::Type::none);

               result.services.emplace_back( ".casual.queue.list.messages",
                     std::bind( &service::list_messages, std::placeholders::_1, std::ref( state)),
                     common::server::Service::Type::cCasualAdmin,
                     common::service::transaction::Type::none);


               result.services.emplace_back( local::service::name::restore(),
                     std::bind( &service::restore, std::placeholders::_1, std::ref( state)),
                     common::server::Service::Type::cCasualAdmin,
                     common::service::transaction::Type::none);

               return result;
            }
         } // admin
      } // broker
   } // queue


} // casual
