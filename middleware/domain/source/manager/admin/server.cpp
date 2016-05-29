//!
//! casual
//!


#include "domain/manager/admin/server.h"
#include "domain/manager/admin/vo.h"
#include "domain/manager/handle.h"
#include "domain/transform.h"


#include "sf/server.h"



namespace casual
{
   using namespace common;

   namespace local
   {
      namespace
      {
         sf::server::type server;
      }
   }

   namespace domain
   {
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



      namespace manager
      {
         namespace admin
         {

            namespace service
            {
               extern "C"
               {

                  void get_state( TPSVCINFO *serviceInfo, manager::State& state)
                  {
                     casual::sf::service::reply::State reply;

                     try
                     {
                        auto service_io = local::server->createService( serviceInfo);

                        auto serviceReturn = casual::domain::transform::state( state);


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

                  void scale_instances( TPSVCINFO *serviceInfo, manager::State& state)
                  {
                     casual::sf::service::reply::State reply;

                     try
                     {
                        auto service_io = local::server->createService( serviceInfo);

                        std::vector< vo::scale::Instances> instances;
                        service_io >> CASUAL_MAKE_NVP( instances);

                        std::vector< vo::scale::Instances> serviceReturn;

                        message::domain::scale::Executable message;

                        for( auto& instance : instances)
                        {
                           auto found = range::find_if( state.executables, [&]( const state::Executable& e){
                              return e.alias == instance.alias;
                           });

                           if( found)
                           {
                              found->configured_instances = instance.instances;
                              message.executables.push_back( found->id);
                              serviceReturn.push_back( std::move( instance));

                           }
                        }

                        //
                        // Push the work-task to our queue, and It'll be processed as soon
                        // as possible
                        //
                        communication::ipc::inbound::device().push( std::move( message));


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


                  void shutdown_domain( TPSVCINFO *serviceInfo, manager::State& state)
                  {
                     casual::sf::service::reply::State reply;

                     try
                     {
                        auto service_io = local::server->createService( serviceInfo);

                        handle::shutdown( state);

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
            } // service


            common::server::Arguments services( manager::State& state)
            {
               common::server::Arguments result{ { common::process::path()}};

               result.services.emplace_back( ".casual.domain.state",
                     std::bind( &service::get_state, std::placeholders::_1, std::ref( state)),
                     common::server::Service::Type::cCasualAdmin,
                     common::server::Service::Transaction::none);

               result.services.emplace_back( ".casual.domain.scale.instances",
                     std::bind( &service::scale_instances, std::placeholders::_1, std::ref( state)),
                     common::server::Service::Type::cCasualAdmin,
                     common::server::Service::Transaction::none);

               result.services.emplace_back( ".casual.domain.shutdown",
                     std::bind( &service::shutdown_domain, std::placeholders::_1, std::ref( state)),
                     common::server::Service::Type::cCasualAdmin,
                     common::server::Service::Transaction::none);

               return result;
            }
         } // admin
      } // manager
   } // gateway
} // casual
