//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/transaction.h"

#include "common/algorithm.h"


namespace casual
{
   namespace configuration
   {
      namespace transaction
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


                  void default_values( Manager& value)
                  {
                     default_values( value.resources, value.manager_default.resource);
                  }
               } // complement

               void validate( const Manager& value)
               {

               }

               template< typename LHS, typename RHS>
               void replace_or_add( LHS& lhs, RHS&& rhs)
               {
                  for( auto& value : rhs)
                  {
                     auto found = common::algorithm::find( lhs, value);

                     if( found)
                        *found = std::move( value);
                     else
                        lhs.push_back( std::move( value));
                  }
               }

            } // <unnamed>
         } // local


         bool operator == ( const Resource& lhs, const Resource& rhs)
         {
            return lhs.name == rhs.name;
         }

         Resource& operator += ( Resource& lhs, const resource::Default& rhs)
         {
            lhs.key = common::coalesce( lhs.key, rhs.key);
            lhs.instances = common::coalesce( lhs.instances, rhs.instances);

            return lhs;
         }


         Manager::Manager() : log{ "${CASUAL_DOMAIN_HOME}/transaction/log.db"}
         {
            manager_default.resource.instances.emplace( 1);
         }


         void Manager::finalize()
         {
            // Complement with default values
            local::complement::default_values( *this);

            // Make sure we've got valid configuration
            local::validate( *this);
         }


         Manager& operator += ( Manager& lhs, const Manager& rhs)
         {
            lhs.log = common::coalesce( rhs.log, lhs.log);

            // defaults just propagates to the left...
            lhs.manager_default = std::move( rhs.manager_default);

            local::replace_or_add( lhs.resources, rhs.resources);

            return lhs;
         }
      } // transaction
   } // configuration



} // casual
