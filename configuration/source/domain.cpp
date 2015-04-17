//!
//! domain.cpp
//!
//! Created on: Aug 3, 2013
//!     Author: Lazan
//!

#include "config/domain.h"
#include "config/file.h"


#include "common/exception.h"
#include "common/file.h"
#include "common/environment.h"
#include "common/algorithm.h"

#include "sf/archive/maker.h"

#include <algorithm>

namespace casual
{
   namespace config
   {
      namespace domain
      {

         namespace local
         {
            namespace
            {
               namespace normalize
               {


                  struct Argument
                  {
                     void operator () ( Executable& executable) const
                     {
                        for( auto& argument : executable.arguments)
                        {
                           argument = common::environment::string( argument);
                        }
                     }
                  };

                  struct Path
                  {
                     void operator () ( Executable& executable) const
                     {
                        executable.path = common::environment::string( executable.path);
                        executable.environment.file = common::environment::string( executable.environment.file);
                        Argument{}( executable);
                     }

                     void operator () ( Default& value) const
                     {
                        value.path = common::environment::string( value.path);
                     }
                  };

                  void path( Domain& domain)
                  {
                     Path{}( domain.casual_default);
                     common::range::for_each( domain.servers, Path{});
                     common::range::for_each( domain.executables, Path{});
                  }
               }

               namespace complement
               {
                  struct Default
                  {
                     Default( const domain::Default& casual_default)
                           : m_casual_default( casual_default)
                     {
                     }

                     void operator ()( domain::Server& server) const
                     {
                        assign_if_empty( server.instances, m_casual_default.server.instances);
                        assign_if_empty( server.alias, nextAlias( server.path));

                        if( ! m_casual_default.path.empty())
                        {
                           if( ! common::file::name::absolute( server.path))
                           {
                              server.path = m_casual_default.path + '/' + server.path;
                           }

                           if( ! server.environment.file.empty() && ! common::file::name::absolute( server.environment.file))
                           {
                              server.environment.file = m_casual_default.path + '/' + server.environment.file;
                           }
                        }
                     }

                     void operator ()( domain::Service& service) const
                     {
                        assign_if_empty( service.timeout, m_casual_default.service.timeout);
                        assign_if_empty( service.transaction, m_casual_default.service.transaction);
                     }

                     void operator ()( domain::Domain& configuration) const
                     {
                        std::for_each( std::begin( configuration.servers), std::end( configuration.servers), *this);
                        std::for_each( std::begin( configuration.services), std::end( configuration.services), *this);

                     }

                  private:

                     inline void assign_if_empty( std::string& value, const std::string& def) const
                     {
                        if( value.empty() || value == "~")
                           value = def;
                     }

                     std::string nextAlias( const std::string& path) const
                     {
                        auto alias = common::file::name::base( path);

                        auto count = m_alias[ alias]++;

                        if( count > 1)
                        {
                           return alias + "_" + std::to_string( count);
                        }

                        return alias;
                     }
                     domain::Default m_casual_default;
                     mutable std::map< std::string, std::size_t> m_alias;
                  };

                  inline void defaultValues( Domain& domain)
                  {
                     Default applyDefaults( domain.casual_default);
                     applyDefaults( domain);
                  }

               } // complement

               void validate( const Domain& settings)
               {

               }

            } //
         } // local

         Domain get( const std::string& file)
         {
            Domain domain;

            //
            // Create the reader and deserialize configuration
            //
            auto reader = sf::archive::reader::from::file( file);

            reader >> CASUAL_MAKE_NVP( domain);

            local::normalize::path( domain);

            //
            // Complement with default values
            //
            local::complement::defaultValues( domain);

            //
            // Make sure we've got valid configuration
            //
            local::validate( domain);

            return domain;

         }

         Domain get()
         {
            const std::string configuration = config::file::domain();

            if( ! configuration.empty())
            {
               return get( configuration);
            }
            else
            {
               throw common::exception::FileNotExist(
                     "could not find domain configuration file - should be: " + config::directory::domain() + "/domain.*");
            }
         }

      } // domain

   } // config
} // casual
