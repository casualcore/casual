//!
//! domain.h
//!
//! Created on: Aug 3, 2013
//!     Author: Lazan
//!

#ifndef DOMAIN_H_
#define DOMAIN_H_


#include "sf/namevaluepair.h"
#include <string>
#include <vector>


namespace casual
{
   namespace config
   {

      namespace domain
      {
         struct Executable
         {
            std::string alias;
            std::string path;
            std::string instances;
            std::string arguments;
            std::vector< std::string> memberships;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( alias);
               archive & CASUAL_MAKE_NVP( path);
               archive & CASUAL_MAKE_NVP( instances);
               archive & CASUAL_MAKE_NVP( arguments);
               archive & CASUAL_MAKE_NVP( memberships);
            }
         };


         struct Server : public Executable
         {
            std::vector< std::string> services;

            template< typename A>
            void serialize( A& archive)
            {
               Executable::serialize( archive);
               archive & CASUAL_MAKE_NVP( services);
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
            std::vector< std::string> dependecies;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( note);
               archive & CASUAL_MAKE_NVP( resource);
               archive & CASUAL_MAKE_NVP( dependecies);
            }
         };


         struct Default
         {
            Default()
            {
               server.instances = std::to_string( 1);
               service.timeout = std::to_string( 3600);
               service.transaction = "auto";
            }

            std::string path;
            Server server;
            Executable executable;
            Service service;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( path);
               archive & CASUAL_MAKE_NVP( server);
               archive & CASUAL_MAKE_NVP( executable);
               archive & CASUAL_MAKE_NVP( service);
            }
         };

         struct Domain
         {

            Default casual_default;
            std::vector< Group> groups;
            std::vector< Server> servers;
            std::vector< Executable> executables;
            std::vector< Service> services;

            template< typename A>
            void serialize( A& archive)
            {
               archive & sf::makeNameValuePair( "default", casual_default);
               archive & CASUAL_MAKE_NVP( groups);
               archive & CASUAL_MAKE_NVP( servers);
               archive & CASUAL_MAKE_NVP( executables);
               archive & CASUAL_MAKE_NVP( services);
            }
         };

         Domain get( const std::string& file);

         //!
         //! Deserialize the domain configuration
         //!
         //! @return domain configuration
         //!
         Domain get();

      } // domain

   } // config
} // casual

#endif // DOMAIN_H_
