//!
//! xa_switch.h
//!
//! Created on: Aug 3, 2013
//!     Author: Lazan
//!

#ifndef XA_SWITCH_H_
#define XA_SWITCH_H_


#include "sf/namevaluepair.h"

#include <vector>
#include <string>


namespace casual
{
   namespace config
   {
      namespace xa
      {
         struct Switch
         {
            std::string key;
            std::string server;
            std::string xa_struct_name;

            std::vector< std::string> libraries;

            struct paths_t
            {
               std::vector< std::string> include;
               std::vector< std::string> library;

               template< typename A>
               void serialize( A& archive)
               {
                  archive & CASUAL_MAKE_NVP( include);
                  archive & CASUAL_MAKE_NVP( library);
               }

            } paths;


            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( key);
               archive & CASUAL_MAKE_NVP( server);
               archive & CASUAL_MAKE_NVP( xa_struct_name);
               archive & CASUAL_MAKE_NVP( libraries);
               archive & CASUAL_MAKE_NVP( paths);
            }
         };

         namespace switches
         {

            std::vector< Switch> get( const std::string& file);

            std::vector< Switch> get();
         } // switch


         void validate( const std::vector< Switch>& switches);


      } // xa
   } // config
} // casual

#endif // XA_SWITCH_H_
