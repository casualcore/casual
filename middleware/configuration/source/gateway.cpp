//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/gateway.h"
#include "configuration/common.h"
#include "configuration/file.h"

#include "common/algorithm.h"

#include "serviceframework/log.h"
#include "common/serialize/create.h"


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
                     default_values( gateway.listeners, gateway.manager_default.listener);
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
                     auto found = algorithm::find( lhs, value);

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
                  algorithm::move( rhs.connections, std::back_inserter( lhs.connections));

                  return lhs;
               }

            } // <unnamed>
         } // local

         Listener& Listener::operator += ( const listener::Default& rhs)
         {
            limit = common::coalesce( limit, rhs.limit);
            return *this;
         }

         bool operator == ( const Listener& lhs, const Listener& rhs)
         {
            return lhs.address == rhs.address;
         }

         Connection& Connection::operator += ( const connection::Default& rhs)
         {
            restart = common::coalesce( std::move( restart), rhs.restart);
            address = common::coalesce( std::move( address), rhs.address);
            return *this;
         }

         Manager& Manager::operator += ( const Manager& rhs)
         {
            return local::append( *this, rhs);
         }

         Manager& Manager::operator += ( Manager&& rhs)
         {
            return local::append( *this, std::move( rhs));
         }

         Manager operator + ( Manager lhs, const Manager& rhs)
         {
            lhs += rhs;
            return lhs;
         }

         void Manager::finalize()
         {
            // Complement with default values
            local::complement::default_values( *this);

            // Make sure we've got valid configuration
            local::validate( *this);
         }


      } // gateway
   } // config
} // casual
