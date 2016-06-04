//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_DOMAIN_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_DOMAIN_H_

#include "common/marshal/marshal.h"
#include "common/uuid.h"

#include <string>
#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace domain
      {
         struct Identity
         {
            Identity();
            Identity( const Uuid& id, std::string name);
            explicit Identity( std::string name);

            Uuid id;
            std::string name;


            friend std::ostream& operator << ( std::ostream& out, const Identity& value);

            CASUAL_CONST_CORRECT_MARSHAL({
               archive & id;
               archive & name;
            })
         };

         const Identity& identity();

         //!
         //! sets the domain identity
         //! Should only be used by casual-domain
         //!
         //! @param value the new identity
         //!
         void identity( Identity value);

      } // domain
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_DOMAIN_H_
