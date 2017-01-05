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

               namespace scale
               {
                  std::vector< vo::scale::Instances> instances( manager::State& state, std::vector< vo::scale::Instances> instances)
                  {
                     std::vector< vo::scale::Instances> result;

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
                           result.push_back( std::move( instance));

                        }
                     }

                     //
                     // Push the work-task to our queue, and It'll be processed as soon
                     // as possible
                     //
                     communication::ipc::inbound::device().push( std::move( message));

                     return result;
                  }

               } // scale

               extern "C"
               {

                  void get_state( TPSVCINFO *serviceInfo, manager::State& state)
                  {
                     casual::sf::service::reply::State reply;

                     try
                     {
                        auto service_io = local::server->createService( serviceInfo);

                        manager::admin::vo::State (*function)(const manager::State&) = casual::domain::transform::state;

                        auto serviceReturn = service_io.call( function, state);


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



                  void scale_instances( TPSVCINFO *service_info, manager::State& state)
                  {
                     casual::sf::service::reply::State reply;

                     try
                     {
                        auto service_io = local::server->createService( service_info);

                        std::vector< vo::scale::Instances> instances;
                        service_io >> CASUAL_MAKE_NVP( instances);

                        auto serviceReturn = service_io.call( &scale::instances, state, std::move( instances));

                        service_io << CASUAL_MAKE_NVP( serviceReturn);
                        reply = service_io.finalize();

                     }
                     catch( ...)
                     {
                        local::server->handleException( service_info, reply);
                     }

                     tpreturn(
                        reply.value,
                        reply.code,
                        reply.data,
                        reply.size,
                        reply.flags);
                  }


                  void shutdown_domain( TPSVCINFO *service_info, manager::State& state)
                  {
                     casual::sf::service::reply::State reply;

                     try
                     {
                        auto service_io = local::server->createService( service_info);

                        service_io.call( &handle::shutdown, state);

                        reply = service_io.finalize();

                     }
                     catch( ...)
                     {
                        local::server->handleException( service_info, reply);
                     }

                     tpreturn(
                        reply.value,
                        reply.code,
                        reply.data,
                        reply.size,
                        reply.flags);
                  }

                  void persist_configuration( TPSVCINFO *service_info, manager::State& state)
                  {
                     casual::sf::service::reply::State reply;

                     try
                     {
                        auto service_io = local::server->createService( service_info);

                        //service_io.call( &configuration::domain::persistent::save, state.configuration);

                        reply = service_io.finalize();

                        casual::domain::log << "reply: " << reply << '\n';

                     }
                     catch( ...)
                     {
                        local::server->handleException( service_info, reply);
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
                     common::service::transaction::Type::none);

               result.services.emplace_back( ".casual.domain.scale.instances",
                     std::bind( &service::scale_instances, std::placeholders::_1, std::ref( state)),
                     common::server::Service::Type::cCasualAdmin,
                     common::service::transaction::Type::none);

               result.services.emplace_back( ".casual.domain.shutdown",
                     std::bind( &service::shutdown_domain, std::placeholders::_1, std::ref( state)),
                     common::server::Service::Type::cCasualAdmin,
                     common::service::transaction::Type::none);

               result.services.emplace_back( ".casual/domain/configuration/persist",
                     std::bind( &service::persist_configuration, std::placeholders::_1, std::ref( state)),
                     common::server::Service::Type::cCasualAdmin,
                     common::service::transaction::Type::none);

               return result;
            }
         } // admin
      } // manager
   } // gateway
} // casual
