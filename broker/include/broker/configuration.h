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



         struct Server
         {
            std::string path;
            std::string instances;
            std::string arguments;
            std::vector< std::string> membership;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( path);
               archive & CASUAL_MAKE_NVP( instances);
               archive & CASUAL_MAKE_NVP( arguments);
               archive & CASUAL_MAKE_NVP( membership);
            }

         };


         struct Service
         {
            std::string name;
            std::string timeout;
            std::string note;
            std::string transaction;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( timeout);
               archive & CASUAL_MAKE_NVP( note);
               archive & CASUAL_MAKE_NVP( transaction);
            }
         };

         struct Resource
         {
            std::string key;
            std::string instances;

            std::string openinfo;
            std::string closeinfo;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( key);
               archive & CASUAL_MAKE_NVP( instances);
               archive & CASUAL_MAKE_NVP( openinfo);
               archive & CASUAL_MAKE_NVP( closeinfo);
            }

         };

         struct Group
         {
            std::string name;
            std::string note;

            Resource resource;


            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( note);
               archive & CASUAL_MAKE_NVP( resource);
            }
         };


         struct Default
         {
            Default()
            {
               server.instances = std::to_string( 1);
               service.timeout = std::to_string( 3600);
            }

            std::string path;
            Server server;
            Service service;
            Group group;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( path);
               archive & CASUAL_MAKE_NVP( server);
               archive & CASUAL_MAKE_NVP( service);
               archive & CASUAL_MAKE_NVP( group);
            }
         };

         struct Settings
         {
            Default casual_default;
            std::vector< Group> groups;
            std::vector< Server> servers;
            std::vector< Service> services;

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
