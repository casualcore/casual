//!
//! casual
//!

#ifndef CONFIGURATION_XA_SWITCH_H_
#define CONFIGURATION_XA_SWITCH_H_


#include "sf/namevaluepair.h"
#include "sf/platform.h"

#include <vector>
#include <string>


namespace casual
{
   namespace configuration
   {
      namespace resource
      {

         struct Property
         {
            Property();
            Property( std::function< void(Property&)> foreign);

            std::string key;
            std::string server;
            std::string xa_struct_name;

            std::vector< std::string> libraries;

            struct
            {
               std::vector< std::string> include;
               std::vector< std::string> library;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( include);
                  archive & CASUAL_MAKE_NVP( library);
               )

            } paths;

            sf::optional< std::string> note;


            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( key);
               archive & CASUAL_MAKE_NVP( server);
               archive & CASUAL_MAKE_NVP( xa_struct_name);
               archive & CASUAL_MAKE_NVP( libraries);
               archive & CASUAL_MAKE_NVP( paths);
               archive & CASUAL_MAKE_NVP( note);
            )
         };

         namespace property
         {
            std::vector< resource::Property> get( const std::string& file);

            std::vector< resource::Property> get();

         } // property


         void validate( const std::vector< Property>& properties);

      } // resource
   } // config
} // casual

#endif // XA_SWITCH_H_
