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

#include "common/code/raise.h"
#include "common/code/casual.h"

#include "common/serialize/create.h"

#include <algorithm>


namespace casual
{
   using namespace common;

   namespace configuration
   {
      namespace domain
      {
         inline namespace v1 {

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

               void validate( const Manager& configuration)
               {
                  // make sure a group is only defined once
                  {
                     auto groups = range::to_reference( configuration.groups);
                     auto duplicates = algorithm::duplicates( algorithm::sort( groups));

                     if( duplicates)
                     {
                        auto names = algorithm::transform( duplicates, []( auto& g){ return g.get().name;});
                        code::raise::error( code::casual::invalid_configuration, "the following groups are defined more than once: ", names);
                     }
                  }

                  // make sure servers and executables has a PATH
                  {
                     auto validate_path = []( auto& e)
                     {
                        if( e.path.empty())
                           code::raise::error( code::casual::invalid_configuration,  "servers and executables need to have a path");
                     };

                     algorithm::for_each( configuration.executables, validate_path);
                     algorithm::for_each( configuration.servers, validate_path);
                  }

                  // make sure we have unique aliases
                  {
                     auto executables = range::to_reference( configuration.executables);

                     // add servers to the same range
                     algorithm::copy( configuration.servers, std::back_inserter( executables));

                     auto has_alias = []( const Executable& e){
                        return e.alias.has_value() && ! e.alias.value().empty();
                     };

                     auto interesting = algorithm::filter( executables, has_alias);

                     auto alias_less = []( const Executable& l, const Executable& r){
                        return l.alias < r.alias;
                     };
                     auto alias_equal = []( const Executable& l, const Executable& r){
                        return l.alias == r.alias;
                     };

                     auto duplicates = algorithm::duplicates( algorithm::sort( interesting, alias_less), alias_equal);

                     if( duplicates)
                     {
                        auto aliases = algorithm::transform( duplicates, []( const Executable& e){ return e.alias.value();});
                        code::raise::error( code::casual::invalid_configuration, "defined aliases are used more than once: ", aliases);
                     }
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

                  algorithm::append( rhs.groups, lhs.groups);
                  algorithm::append( rhs.executables, lhs.executables);
                  algorithm::append( rhs.servers, lhs.servers);
                  algorithm::append( rhs.services, lhs.services);

                  return lhs;
               }

               Manager get( Manager current, const std::string& file)
               {
                  Manager domain;
                  // Create the archive and deserialize configuration
                  common::file::Input stream( file);
                  auto archive = common::serialize::create::reader::consumed::from( stream.extension(), stream);
                  archive >> CASUAL_NAMED_VALUE( domain);

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

            // apply file::find, if user using patterns
            auto domain = algorithm::accumulate( common::file::find( files), Manager{}, &local::get);

            // finalize the complete domain configuration
            finalize( domain);

            log::line( verbose::log, "domain: ", domain);

            return domain;

         }
         
      } // v1
      } // domain

   } // configuration
} // casual
