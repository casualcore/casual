//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_SERVICE_H_
#define CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_SERVICE_H_


#include "sf/namevaluepair.h"
#include "sf/platform.h"

namespace casual
{
   namespace configuration
   {
      namespace service
      {

         namespace service
         {
            struct Default
            {
               sf::optional< std::string> timeout;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( timeout);
               )

            };
         } // service


         struct Service : service::Default
         {

            Service();
            Service( std::function< void(Service&)> foreign);

            std::string name;
            std::string note;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               service::Default::serialize( archive);
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( note);
            )

            friend bool operator == ( const Service& lhs, const Service& rhs);

            //!
            //! Will assign any unassigned values in lhs
            //!
            friend Service& operator += ( Service& lhs, const service::Default& rhs);
         };

      } // service
   } // configuration
} // casual

#endif // CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_SERVICE_H_
