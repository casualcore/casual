//!
//! casual
//!

#include "configuration/build/server.h"

#include "sf/archive/maker.h"

namespace casual
{

   namespace configuration
   {
      namespace build
      {
         namespace server
         {
            namespace local
            {
               namespace
               {
                  namespace complement
                  {
                     void default_values( Server& server)
                     {
                        for( auto&& service : server.services)
                        {
                           if( ! service.function) service.function.emplace( service.name);

                           service.transaction = common::coalesce( service.transaction, server.server_default.service.transaction);
                           service.type = common::coalesce( service.type, server.server_default.service.type);
                        }
                     }
                  } // complement
               } // <unnamed>
            } // local



            Service::Service() = default;
            Service::Service( std::function< void(Service&)> foreign) { foreign( *this);}


            namespace server
            {
               Default::Default()
               {
                  service.transaction.emplace( "auto");
                  service.type.emplace( 0);
               }
            } // service

            Server get( const std::string& file)
            {
               Server server;

               //
               // Create the reader and deserialize configuration
               //
               auto reader = sf::archive::reader::from::file( file);
               reader >> CASUAL_MAKE_NVP( server);

               //
               // Complement with default values
               //
               local::complement::default_values( server);


               return server;

            }
         } // server
      } // build
   } // config
} // casual
