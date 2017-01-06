//!
//! casual 
//!

#include "configuration/server.h"

#include "common/algorithm.h"

namespace casual
{
   using namespace common;

   namespace configuration
   {
      namespace server
      {
         namespace local
         {
            namespace
            {
               template< typename T>
               void assign_if_empty( sf::optional< T>& value, const sf::optional< T>& optional)
               {
                  if( ! value.has_value())
                     value = optional;
               }

            } // <unnamed>
         } // local

         Executable::Executable() = default;
         Executable::Executable( std::function< void(Executable&)> foreign)
         {
            foreign( *this);
         }

         bool operator == ( const Executable& lhs, const Executable& rhs)
         {
            return lhs.alias.value_or( lhs.path) == rhs.alias.value_or( rhs.path);
         }

         Executable& operator += ( Executable& lhs, const executable::Default& rhs)
         {
            local::assign_if_empty( lhs.instances, rhs.instances);
            local::assign_if_empty( lhs.restart, rhs.restart);
            local::assign_if_empty( lhs.memberships, rhs.memberships);
            local::assign_if_empty( lhs.environment, rhs.environment);

            return lhs;
         }


         Server::Server() = default;
         Server::Server( std::function< void(Server&)> foreign) { foreign( *this);}


         bool operator == ( const Server& lhs, const Server& rhs)
         {
            return lhs.restriction == rhs.restriction &&
                  static_cast< const Executable&>( lhs) == static_cast< const Executable&>( rhs);
         }



      } // server

   } // configuration



} // casual
