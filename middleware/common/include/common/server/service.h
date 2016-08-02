//!
//! casual
//!

#ifndef CASUAL_COMMON_SERVER_SERVICE_H_
#define CASUAL_COMMON_SERVER_SERVICE_H_

#include "common/service/type.h"

#include "xatmi.h"

#include <functional>
#include <string>
#include <ostream>
#include <cstdint>

namespace casual
{
   namespace common
   {
      namespace server
      {
         struct Service
         {
            enum Type : std::uint64_t
            {
               cXATMI = 0,
               cCasualAdmin = 10,
               cCasualSF = 11,
            };



            using function_type = std::function< void( TPSVCINFO*)>;


            Service( std::string name, function_type function, std::uint64_t type, service::transaction::Type transaction);
            Service( std::string name, function_type function);

            Service( Service&&);
            Service& operator = ( Service&&);


            void call( TPSVCINFO* serviceInformation);

            std::string origin;
            function_type function;

            std::uint64_t type = Type::cXATMI;
            service::transaction::Type transaction = service::transaction::Type::automatic;
            bool active = true;

            friend std::ostream& operator << ( std::ostream& out, const Service& service);

            friend bool operator == ( const Service& lhs, const Service& rhs);
            friend bool operator != ( const Service& lhs, const Service& rhs);


         private:
            typedef void(*const* target_type)(TPSVCINFO*);
            target_type adress() const;


         };

      } // server
   } // common
} // casual

#endif // SERVICE_H_
