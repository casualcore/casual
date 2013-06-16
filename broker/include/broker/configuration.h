//!
//! casual_broker_configuration.h
//!
//! Created on: Nov 4, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BROKER_CONFIGURATION_H_
#define CASUAL_BROKER_CONFIGURATION_H_

#include "sf/namevaluepair.h"

//
// std
//
#include <limits>
#include <list>
#include <string>
#include <vector>

namespace casual
{
   namespace broker
   {

      namespace configuration
      {

         struct Limit
         {

            std::string min;
            std::string max;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( min);
               archive & CASUAL_MAKE_NVP( max);
            }
         };

         struct Server
         {
            std::string path;
            std::string instances;
            Limit limits;
            std::string arguments;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( path);
               archive & CASUAL_MAKE_NVP( instances);
               archive & CASUAL_MAKE_NVP( limits);
               archive & CASUAL_MAKE_NVP( arguments);
            }

         };



         struct Service
         {
            std::string name;
            std::string timeout;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( timeout);
            }
         };

         struct Group
         {
            std::string name;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( name);
            }
         };


         struct Default
         {
            Default()
            {
               server.limits.max = std::to_string( std::numeric_limits< std::size_t>::max());
               server.limits.min = std::to_string( 0);
               server.instances = std::to_string( 1);
               service.timeout = std::to_string( 3600);
            }

            Server server;
            Service service;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( server);
               archive & CASUAL_MAKE_NVP( service);
            }
         };

         struct Settings
         {
            Default casual_default;
            std::vector< Group> groups;
            std::list< Server> servers;
            std::list< Service> services;

            template< typename A>
            void serialize( A& archive)
            {
               archive & sf::makeNameValuePair( "default", casual_default);
               archive & CASUAL_MAKE_NVP( groups);
               archive & CASUAL_MAKE_NVP( servers);
               archive & CASUAL_MAKE_NVP( services);
            }
         };

         namespace complement
         {
            inline void assign_if_empty( std::string& value, const std::string& def)
            {
               if( value.empty()) value = def;
            }

            struct Default
            {
               Default( const configuration::Default& casual_default) : m_casual_default( casual_default) {}

               void operator () ( configuration::Server& server) const
               {
                  assign_if_empty( server.instances, m_casual_default.server.instances);
                  assign_if_empty( server.limits.min, m_casual_default.server.limits.min);
                  assign_if_empty( server.limits.max, m_casual_default.server.limits.max);
               }

               void operator () ( configuration::Service& service) const
               {
                  assign_if_empty( service.timeout,  m_casual_default.service.timeout);
               }

            private:
               configuration::Default m_casual_default;
            };
         } // complement


         inline void validate( const Settings& settings)
         {

         }


      } // configuration
   } // broker
} // casual


#endif /* CASUAL_BROKER_CONFIGURATION_H_ */
