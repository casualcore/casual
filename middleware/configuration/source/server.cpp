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

               void assign_if_empty( std::string& value, const std::string& def)
               {
                  if( value.empty() || value == "~")
                     value = def;
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
            return coalesce( lhs.alias, lhs.path) == coalesce( rhs.alias, rhs.path);
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



         namespace complement
         {


            Alias::Alias( std::map< std::string, std::size_t>& used) : m_used{ used}
            {

            }

            void Alias::operator () ( Executable& value)
            {
               value.alias = coalesce( value.alias, common::file::name::base( value.path));

               auto count = m_used.get()[ value.alias]++;

               if( count > 1)
               {
                  value.alias +=  "_" + std::to_string( count);
               }
            }

         } // complement

      } // server

   } // configuration



} // casual
