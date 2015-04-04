//!
//! service.cpp
//!
//! Created on: Apr 4, 2015
//!     Author: Lazan
//!

#include "common/server/service.h"



#include <map>

namespace casual
{
   namespace common
   {
      namespace server
      {


         Service::Service( std::string name, function_type function, std::uint64_t type, Transaction transaction)
            : name( std::move( name)), function( function), type( type), transaction( transaction), m_adress( *function.target< adress_type>()) {}

         Service::Service( std::string name, function_type function)
            : Service( std::move( name), std::move( function), Type::cXATMI, Transaction::automatic) {}


         Service::Service( Service&&) = default;



         void Service::call( TPSVCINFO* serviceInformation)
         {
            function( serviceInformation);
         }


         std::ostream& operator << ( std::ostream& out, const Service& service)
         {
            return out << "{name: " << service.name << " type: " << service.type << " transaction: " << static_cast< std::uint64_t>( service.transaction)
                  << " active: " << service.active << "};";
         }

         bool operator == ( const Service& lhs, const Service& rhs)
         {
            return lhs.m_adress == rhs.m_adress;
         }

         bool operator != ( const Service& lhs, const Service& rhs)
         {
            return lhs.m_adress != rhs.m_adress;
         }


         namespace service
         {
            namespace transaction
            {
               Service::Transaction mode( const std::string& mode)
               {
                  const static std::map< std::string, Service::Transaction> mapping{
                     { "automatic", Service::Transaction::automatic},
                     { "auto", Service::Transaction::automatic},
                     { "join", Service::Transaction::join},
                     { "atomic", Service::Transaction::atomic},
                     { "none", Service::Transaction::none},
                  };
                  return mapping.at( mode);
               }

            } // transaction

         } // service

      } // server
   } // common
} // casual
