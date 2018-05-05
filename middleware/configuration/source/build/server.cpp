//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/build/server.h"

#include "serviceframework/archive/maker.h"

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
                           service.category = common::coalesce( service.category, server.server_default.service.category);
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
               }
            } // service

            Server get( const std::string& file)
            {
               Server server;

               //
               // Create the reader and deserialize configuration
               //
               auto reader = serviceframework::archive::reader::from::file( file);
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
