//!
//! serverdefinition.cpp
//!
//! Created on: May 10, 2014
//!     Author: Lazan
//!

#include "config/serverdefinition.h"


#include "sf/archive_maker.h"

namespace casual
{

   namespace config
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
                     void setIfEmpty( std::string& variable, const std::string& value)
                     {
                        if( variable.empty()) variable = value;
                     }

                     void defaultValues( Server& server)
                     {
                        for( auto&& service : server.services)
                        {
                           setIfEmpty( service.function, service.name);
                           setIfEmpty( service.transaction, server.transaction);
                           setIfEmpty( service.type, server.type);
                        }

                     }
                  } // complement
               } // <unnamed>
            } // local

            Server get( const std::string& file)
            {
               Server server;

               //
               // Create the reader and deserialize configuration
               //
               auto reader = sf::archive::reader::makeFromFile( file);

               reader >> CASUAL_MAKE_NVP( server);

               //
               // Complement with default values
               //
               local::complement::defaultValues( server);


               return server;

            }
         } // server
      } // build
   } // config
} // casual
