//!
//! casual
//!


#include "gateway/manager/admin/server.h"
#include "gateway/manager/admin/vo.h"
#include "gateway/transform.h"


#include "sf/server.h"


#include "xatmi.h"

namespace casual
{
   namespace local
   {
      namespace
      {
         sf::Server server;
      }
   }

   namespace gateway
   {

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
                        auto service_io = local::server.service( *serviceInfo);

                        manager::admin::vo::State (*function)( const manager::State& state) = &gateway::transform::state;

                        auto serviceReturn = service_io.call( function, state);

                        service_io << CASUAL_MAKE_NVP( serviceReturn);

                        reply = service_io.finalize();

                     }
                     catch( ...)
                     {
                        local::server.exception( *serviceInfo, reply);
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
               common::server::Arguments result{ { common::process::path()}, nullptr, nullptr};

               result.services = {
                     common::server::xatmi::service( ".casual.gateway.state",
                        std::bind( &service::get_state, std::placeholders::_1, std::ref( state)),
                        common::service::transaction::Type::none,
                        common::service::category::admin)
               };


               return result;
            }
         } // admin
      } // manager
   } // gateway
} // casual
