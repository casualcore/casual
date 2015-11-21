//!
//! xa_switch.cpp
//!
//! Created on: Aug 3, 2013
//!     Author: Lazan
//!

#include "config/xa_switch.h"

#include "common/exception.h"
#include "common/environment.h"

#include "sf/archive/maker.h"



namespace casual
{
   namespace config
   {
      namespace xa
      {

         namespace switches
         {

            std::vector< Switch> get( const std::string& file)
            {
               std::vector< Switch> resources;

               //
               // Create the reader and deserialize configuration
               //
               auto reader = sf::archive::reader::from::file( file);

               reader >> CASUAL_MAKE_NVP( resources);

               //
               // Make sure we've got valid configuration
               //
               config::xa::validate( resources);

               return resources;
            }

            std::vector< Switch> get()
            {
               //
               // Try to find configuration file
               //
               const std::string configFile = common::environment::file::installedConfiguration();

               if( ! configFile.empty())
               {
                  return get( configFile);
               }
               else
               {
                  throw common::exception::invalid::File( "could not find resource configuration file",
                        common::exception::make_nip( "path", common::environment::directory::casual() + "/configuration"),
                        common::exception::make_nip( "name", "resources.(yaml|json|xml|..."));
               }
            }
         } // switches

         void validate( const std::vector< Switch>& switches)
         {
         }

      } // xa
   } // config
} // casual
