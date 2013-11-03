//!
//! domain.cpp
//!
//! Created on: Aug 3, 2013
//!     Author: Lazan
//!

#include "config/domain.h"

#include "common/environment.h"
#include "common/exception.h"

#include "sf/archive_maker.h"

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
                        assign_if_empty( server.alias, nextAlias());
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

                     static std::string nextAlias()
                     {
                        static long index = 1;
                        return std::to_string( index++);
                     }
                     domain::Default m_casual_default;
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
            auto reader = sf::archive::reader::makeFromFile( file);

            reader >> CASUAL_MAKE_NVP( domain);

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
            const std::string configFile = common::environment::file::configuration();

            if( !configFile.empty())
            {
               return get( configFile);
            }
            else
            {
               throw common::exception::FileNotExist(
                     "could not find domain configuration file - should be: " + common::environment::directory::domain() + "/configuration/domain.*");
            }
         }

      } // domain

   } // config
} // casual
