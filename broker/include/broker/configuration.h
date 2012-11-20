//!
//! casual_broker_configuration.h
//!
//! Created on: Nov 4, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BROKER_CONFIGURATION_H_
#define CASUAL_BROKER_CONFIGURATION_H_

#include "sf/archive_base.h"

#include <limits>

#include <list>

namespace casual
{
   namespace broker
   {

      namespace configuration
      {
         enum
         {
            cUnset = -1
         };

         struct Limit
         {

            int min = cUnset;
            int max = cUnset;

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
            int instances = cUnset;
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
            int timeout = cUnset;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( timeout);
            }
         };


         struct Default
         {
            Default()
            {
               server.instances = 1;
               server.limits.min = 0;
               server.limits.max = 10;

               service.timeout = 90;
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
            std::list< Server> servers;
            std::list< Service> services;

            template< typename A>
            void serialize( A& archive)
            {
               archive & sf::makeNameValuePair( "default", casual_default);
               archive & CASUAL_MAKE_NVP( servers);
               archive & CASUAL_MAKE_NVP( services);
            }


         };

         namespace complement
         {
            struct Default
            {
               Default( const configuration::Default& casual_default) : m_casual_default( casual_default) {}

               void operator () ( configuration::Server& server) const
               {
                  if( server.instances == cUnset) server.instances = m_casual_default.server.instances;
                  if( server.limits.min == cUnset) server.limits.min = m_casual_default.server.limits.min;
                  if( server.limits.max == cUnset) server.limits.max = m_casual_default.server.limits.max;
               }

               void operator () ( configuration::Service& service) const
               {
                  if( service.timeout == cUnset) service.timeout = m_casual_default.service.timeout;
               }

            private:
               configuration::Default m_casual_default;
            };
         } // complement

         namespace validate
         {

         }

      } // configuration
   } // broker
} // casual


#endif /* CASUAL_BROKER_CONFIGURATION_H_ */
