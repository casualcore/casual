//!
//! casual 
//!

#include "configuration/gateway.h"
#include "configuration/common.h"
#include "configuration/file.h"

#include "common/algorithm.h"

#include "sf/log.h"
#include "sf/archive/maker.h"


namespace casual
{
   using namespace common;
   namespace configuration
   {
      namespace gateway
      {

         namespace local
         {
            namespace
            {

               namespace complement
               {
                  template< typename R, typename V>
                  void default_values( R& range, V&& value)
                  {
                     for( auto& element : range) { element += value;}
                  }

                  inline void default_values( gateway::Manager& gateway)
                  {
                     default_values( gateway.connections, gateway.manager_default.connection);
                  }

               } // complement

               void validate( const gateway::Manager& value)
               {

               }


               template< typename LHS, typename RHS>
               void replace_or_add( LHS& lhs, RHS&& rhs)
               {
                  for( auto& value : rhs)
                  {
                     auto found = range::find( lhs, value);

                     if( found)
                     {
                        *found = std::move( value);
                     }
                     else
                     {
                        lhs.push_back( std::move( value));
                     }
                  }
               }

               template< typename G>
               Manager& append( Manager& lhs, G&& rhs)
               {
                  local::replace_or_add( lhs.listeners, std::move( rhs.listeners));
                  local::replace_or_add( lhs.connections, std::move( rhs.connections));

                  return lhs;
               }


            } // <unnamed>
         } // local

         bool operator == ( const Listener& lhs, const Listener& rhs)
         {
            return lhs.address == rhs.address;
         }

         bool operator == ( const Connection& lhs, const Connection& rhs)
         {
            return lhs.address == rhs.address;
         }

         Connection& operator += ( Connection& lhs, const connection::Default& rhs)
         {
            lhs.restart = common::coalesce( lhs.restart, rhs.restart);
            lhs.type = common::coalesce( lhs.type, rhs.type);
            lhs.address = common::coalesce( lhs.address, rhs.address);

            return lhs;
         }

         namespace manager
         {
            Default::Default()
            {
               connection.type = std::string( "tcp");
               connection.restart = true;
            }
         } // manager


         Manager& Manager::operator += ( const Manager& rhs)
         {
            return local::append( *this, rhs);
         }

         Manager& Manager::operator += ( Manager&& rhs)
         {
            return local::append( *this, std::move( rhs));
         }

         Manager operator + ( const Manager& lhs, const Manager& rhs)
         {
            auto result = lhs;
            result += rhs;
            return result;
         }

         void Manager::finalize()
         {
            //
            // Complement with default values
            //
            local::complement::default_values( *this);

            //
            // Make sure we've got valid configuration
            //
            local::validate( *this);
         }


      } // gateway
   } // config
} // casual
