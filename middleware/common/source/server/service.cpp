//!
//!
//! casual
//!

#include "common/server/service.h"

#include "common/algorithm.h"



namespace casual
{
   namespace common
   {
      namespace server
      {


         Service::Service( std::string name, function_type function, std::string category, service::transaction::Type transaction)
            : origin( std::move( name)), function( function), category( std::move( category)), transaction( transaction) {}

         Service::Service( std::string name, function_type function)
            : Service( std::move( name), std::move( function), "", service::transaction::Type::automatic) {}


         void Service::call( TPSVCINFO* serviceInformation)
         {
            function( serviceInformation);
         }

         namespace local
         {
            namespace
            {
               bool compare( const Service::function_type& lhs, const Service::function_type& rhs)
               {
                  auto lhs_target = lhs.target<void(*)(TPSVCINFO*)>();
                  auto rhs_target = rhs.target<void(*)(TPSVCINFO*)>();

                  if( lhs_target && rhs_target)
                     return *lhs_target == *rhs_target;

                  return lhs_target == rhs_target;
               }

            } // <unnamed>
         } // local


         std::ostream& operator << ( std::ostream& out, const Service& service)
         {
            return out << "{ origin: " << service.origin
                  << ", category: " << service.category
                  << ", transaction: " << service.transaction
                  << '}';
         }

         bool operator == ( const Service& lhs, const Service& rhs)
         {
            return local::compare( lhs.function, rhs.function);
         }

         bool operator == ( const Service& lhs, const Service::function_type& rhs)
         {
            return local::compare( lhs.function, rhs);
         }

         bool operator != ( const Service& lhs, const Service& rhs)
         {
            return ! ( lhs == rhs);
         }

      } // server
   } // common
} // casual
