//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_DOMAIN_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_DOMAIN_H_

#include "common/marshal/marshal.h"
#include "common/uuid.h"
#include "common/file.h"
#include "common/process.h"

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

            CASUAL_CONST_CORRECT_MARSHAL({
               archive & id;
               archive & name;
            })

            friend std::ostream& operator << ( std::ostream& out, const Identity& value);

            friend bool operator == ( const Identity& lhs, const Identity& rhs);
            inline friend bool operator != ( const Identity& lhs, const Identity& rhs) { return ! ( lhs == rhs);}
            friend bool operator < ( const Identity& lhs, const Identity& rhs);

         };

         const Identity& identity();

         //!
         //! sets the domain identity
         //! Should only be used by casual-domain
         //!
         //! @param value the new identity
         //!
         void identity( Identity value);


         namespace singleton
         {
            file::scoped::Path create( const process::Handle& process, const Identity& identity);

            struct Result
            {
               process::Handle process;
               Identity identity;

               friend std::ostream& operator << ( std::ostream& out, const Result& value);
            };

            Result read( process::pattern::Sleep retries);
            Result read();

         } // singleton




      } // domain
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_DOMAIN_H_
