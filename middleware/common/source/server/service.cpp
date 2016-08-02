//!
//! service.cpp
//!
//! casual
//!

#include "common/server/service.h"

#include "common/algorithm.h"



#include <map>

namespace casual
{
   namespace common
   {
      namespace server
      {


         Service::Service( std::string name, function_type function, std::uint64_t type, service::transaction::Type transaction)
            : origin( std::move( name)), function( function), type( type), transaction( transaction) {}

         Service::Service( std::string name, function_type function)
            : Service( std::move( name), std::move( function), Type::cXATMI, service::transaction::Type::automatic) {}


         Service::Service( Service&&) = default;
         Service& Service::operator = ( Service&&) = default;



         void Service::call( TPSVCINFO* serviceInformation)
         {
            function( serviceInformation);
         }

         Service::target_type Service::adress() const
         {
            return function.target<void(*)(TPSVCINFO*)>();
         }


         std::ostream& operator << ( std::ostream& out, const Service& service)
         {
            return out << "{ origin: " << service.origin << " type: " << service.type << " transaction: " << service.transaction
                  << " active: " << service.active << "};";
         }

         bool operator == ( const Service& lhs, const Service& rhs)
         {
            if( lhs.adress() && rhs.adress())
               return *lhs.adress() == *rhs.adress();

            return lhs.adress() == rhs.adress();
         }

         bool operator != ( const Service& lhs, const Service& rhs)
         {
            return ! ( lhs == rhs);
         }

      } // server
   } // common
} // casual
