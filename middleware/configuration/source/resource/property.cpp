//!
//! casual 
//!

#include "configuration/resource/property.h"

#include "sf/archive/maker.h"


#include "common/environment.h"
#include "common/exception/system.h"


namespace casual
{
   namespace configuration
   {
      namespace resource
      {
         Property::Property() = default;
         Property::Property( std::function< void(Property&)> foreign) { foreign( *this);}

         namespace property
         {

            std::vector< Property> get( const std::string& file)
            {
               std::vector< Property> resources;

               //
               // Create the reader and deserialize configuration
               //
               auto reader = sf::archive::reader::from::file( file);

               reader >> CASUAL_MAKE_NVP( resources);

               //
               // Make sure we've got valid configuration
               //
               validate( resources);

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
                        return common::file::find(
                              common::environment::directory::casual() + "/configuration",
                              std::regex( "resources.(yaml|xml|json|ini)" ));
                     }

                     return file;
                  }
               } // <unnamed>
            } // local

            std::vector< Property> get()
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
                  throw common::exception::system::invalid::File( common::string::compose( "could not find resource configuration file",
                        " path: ", common::environment::directory::casual(),  "/configuration",
                        " name: ", "resources.(yaml|json|xml|..."));
               }
            }


         } // property


         void validate( const std::vector< Property>& properties)
         {

         }

      } // resource
   } // config
} // casual

