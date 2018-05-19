//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/domain.h"
#include "configuration/file.h"
#include "configuration/common.h"

#include "common/file.h"
#include "common/environment.h"
#include "common/algorithm.h"

#include "serviceframework/archive/create.h"
#include "serviceframework/log.h"

#include <algorithm>


namespace casual
{
   using namespace common;

   namespace configuration
   {
      namespace domain
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

                  inline void default_values( Manager& domain)
                  {
                     default_values( domain.executables, domain.manager_default.executable);
                     default_values( domain.servers, domain.manager_default.server);
                     default_values( domain.services, domain.manager_default.service);
                  }

               } // complement

               void validate( const Manager& settings)
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

               template< typename D>
               Manager& append( Manager& lhs, D&& rhs)
               {
                  if( lhs.name.empty()) { lhs.name = std::move( rhs.name);}

                  local::replace_or_add( lhs.transaction.resources, rhs.transaction.resources);
                  local::replace_or_add( lhs.groups, rhs.groups);
                  local::replace_or_add( lhs.executables, rhs.executables);
                  local::replace_or_add( lhs.servers, rhs.servers);
                  local::replace_or_add( lhs.services, rhs.services);

                  lhs.gateway += std::move( rhs.gateway);
                  lhs.queue += std::move( rhs.queue);

                  return lhs;
               }

               Manager get( Manager domain, const std::string& file)
               {

                  //
                  // Create the archive and deserialize configuration
                  //
                  common::file::Input stream( file);
                  auto archive = serviceframework::archive::create::reader::consumed::from( stream.extension(), stream);
                  archive >> CASUAL_MAKE_NVP( domain);

                  // validate if the user has stuff that we didn't consume
                  archive.validate();

                  finalize( domain);

                  return domain;

               }

            } // unnamed
         } // local


         namespace manager
         {
            Default::Default()
            {
               server.instances.emplace( 1);
               executable.instances.emplace( 1);
               service.timeout.emplace( "0s");
            }

         } // domain



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

         void finalize( Manager& configuration)
         {
            //
            // Complement with default values
            //
            local::complement::default_values( configuration);

            //
            // Make sure we've got valid configuration
            //
            local::validate( configuration);

            configuration.transaction.finalize();
            configuration.gateway.finalize();
            configuration.queue.finalize();

         }


         Manager get( const std::vector< std::string>& files)
         {
            common::Trace trace{ "configuration::domain::get"};

            auto domain = algorithm::accumulate( files, Manager{}, &local::get);

            return domain;

         }
      } // domain

   } // configuration
} // casual
