//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/build/server.h"
#include "configuration/common.h"

#include "common/serialize/create.h"
#include "common/serialize/line.h"
#include "common/file.h"

#include "common/algorithm/coalesce.h"

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

                           service.transaction = common::algorithm::coalesce( service.transaction, server.server_default.service.transaction);
                           service.category = common::algorithm::coalesce( service.category, server.server_default.service.category);
                        }
                     }
                  } // complement
               } // <unnamed>
            } // local

            namespace server
            {
               Default::Default()
               {
                  service.transaction.emplace( "auto");
               }
            } // service

            Server get( const std::string& name)
            {
               Server server;

               // Create the reader and deserialize configuration
               common::file::Input file{ name};
               auto reader = common::serialize::create::reader::consumed::from( file);

               reader >> CASUAL_NAMED_VALUE( server);

               reader.validate();

               // Complement with default values
               local::complement::default_values( server);

               common::log::line( verbose::log, "server: ", server);

               return server;

            }
         } // server
      } // build
   } // config
} // casual
