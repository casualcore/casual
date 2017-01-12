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
      namespace service
      {
         namespace category
         {
            constexpr const char* none = "";
            constexpr const char* admin = ".admin";
         } // category
      } // service

      namespace server
      {

         struct Service
         {

            using function_type = std::function< void( TPSVCINFO*)>;

            Service( std::string name, function_type function, std::string category, service::transaction::Type transaction);
            Service( std::string name, function_type function);

            Service( Service&&);
            Service& operator = ( Service&&);


            void call( TPSVCINFO* serviceInformation);

            std::string origin;
            function_type function;

            std::string category;

            service::transaction::Type transaction = service::transaction::Type::automatic;

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
