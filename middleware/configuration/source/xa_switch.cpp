//!
//! casual
//!

#include "configuration/xa_switch.h"

#include "common/exception.h"
#include "common/environment.h"

#include "sf/archive/maker.h"



namespace casual
{
   namespace configuration
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
               configuration::xa::validate( resources);

               return resources;
            }

            namespace local
            {
               namespace
               {
                  std::string file()
                  {
                     auto file = common::environment::variable::get( "CASUAL_RESOURCE_CONFIGURATION_FILE", "");

                     if( file.empty())
                     {
                        return common::environment::file::installedConfiguration();
                     }

                     return file;
                  }
               } // <unnamed>
            } // local

            std::vector< Switch> get()
            {
               //
               // Try to find configuration file
               //
               const auto file = local::file();

               if( ! file.empty())
               {
                  return get( file);
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
