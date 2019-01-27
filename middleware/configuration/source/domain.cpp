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
                        *found = std::move( value);
                     else
                        lhs.push_back( std::move( value));
                  }
               }

               template< typename LHS, typename RHS>
               void compound_assign_or_add( LHS& lhs, RHS&& rhs)
               {
                  for( auto& value : rhs)
                  {
                     auto found = algorithm::find( lhs, value);

                     if( found)
                        *found += std::move( value);
                     else
                        lhs.push_back( std::move( value));
                  }
               }

               template< typename D>
               Manager& append( Manager& lhs, D&& rhs)
               {
                  lhs.name = coalesce( std::move( rhs.name), std::move( lhs.name));

                  lhs.manager_default += std::move( rhs.manager_default);

                  lhs.transaction += std::move( rhs.transaction);
                  lhs.gateway += std::move( rhs.gateway);
                  lhs.queue += std::move( rhs.queue);

                  local::compound_assign_or_add( lhs.groups, rhs.groups);

                  local::replace_or_add( lhs.executables, rhs.executables);
                  local::replace_or_add( lhs.servers, rhs.servers);
                  local::replace_or_add( lhs.services, rhs.services);

                  return lhs;
               }

               Manager get( Manager current, const std::string& file)
               {
                  Manager domain;
                  // Create the archive and deserialize configuration
                  common::file::Input stream( file);
                  auto archive = serviceframework::archive::create::reader::consumed::from( stream.extension(), stream);
                  archive >> CASUAL_MAKE_NVP( domain);

                  // validate if the user has stuff that we didn't consume
                  archive.validate();

                  finalize( domain);

                  // accumulate it to current
                  current += std::move( domain);

                  return current;

               }

            } // unnamed
         } // local


         namespace manager
         {
            Default& Default::operator += ( const Default& rhs)
            {
               // we accumulate environment
               environment += rhs.environment;
               
               service = rhs.service;
               executable = rhs.executable;
               server = rhs.server;               
               
               return *this;
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

         Manager operator + ( Manager lhs, const Manager& rhs)
         {
            lhs += rhs;
            return lhs;
         }

         void finalize( Manager& configuration)
         {
            // Complement with default values
            local::complement::default_values( configuration);

            // Make sure we've got valid configuration
            local::validate( configuration);

            configuration.transaction.finalize();
            configuration.gateway.finalize();
            configuration.queue.finalize();
         }


         Manager get( const std::vector< std::string>& files)
         {
            Trace trace{ "configuration::domain::get"};

            auto domain = algorithm::accumulate( files, Manager{}, &local::get);

            // finalize the complete domain configuration
            finalize( domain);

            log::line( verbose::log, "domain: ", domain);

            return domain;

         }
      } // domain

   } // configuration
} // casual
