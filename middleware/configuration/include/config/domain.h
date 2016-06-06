//!
//! casual
//!

#ifndef CONFIG_DOMAIN_H_
#define CONFIG_DOMAIN_H_


#include "config/environment.h"
#include "config/gateway.h"


#include "sf/namevaluepair.h"

#include <algorithm>
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
            std::string note;
            std::string alias;
            std::string path;
            std::string instances;
            std::string restart;
            std::vector< std::string> arguments;
            std::vector< std::string> memberships;

            Environment environment;


            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( note);
               archive & CASUAL_MAKE_NVP( alias);
               archive & CASUAL_MAKE_NVP( path);
               archive & CASUAL_MAKE_NVP( instances);
               archive & CASUAL_MAKE_NVP( restart);
               archive & CASUAL_MAKE_NVP( arguments);
               archive & CASUAL_MAKE_NVP( memberships);
               archive & CASUAL_MAKE_NVP( environment);
            )

            friend bool operator == ( const Executable& lhs, const Executable& rhs);
         };

         namespace transaction
         {
            struct Manager
            {
               std::string path = "casual-transaction-manager";
               std::string database = "transaction-manager.db";

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( path);
                  archive & CASUAL_MAKE_NVP( database);
               )
            };

         } // transaction


         struct Server : public Executable
         {
            std::vector< std::string> restriction;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               Executable::serialize( archive);
               archive & CASUAL_MAKE_NVP( restriction);
            )
         };


         struct Service
         {
            std::string name;
            std::string timeout;
            std::string note;
            std::string transaction;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( timeout);
               archive & CASUAL_MAKE_NVP( note);
               archive & CASUAL_MAKE_NVP( transaction);
            )

            friend bool operator == ( const Service& lhs, const Service& rhs);
         };

         struct Resource
         {
            std::string key;
            std::string instances;

            std::string openinfo;
            std::string closeinfo;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( key);
               archive & CASUAL_MAKE_NVP( instances);
               archive & CASUAL_MAKE_NVP( openinfo);
               archive & CASUAL_MAKE_NVP( closeinfo);
            )

         };

         struct Group
         {
            std::string name;
            std::string note;

            std::vector< Resource> resources;
            std::vector< std::string> dependencies;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( note);
               archive & CASUAL_MAKE_NVP( resources);
               archive & CASUAL_MAKE_NVP( dependencies);
            )

            friend bool operator == ( const Group& lhs, const Group& rhs);
         };


         struct Default
         {
            Default()
            {
               server.instances = std::to_string( 1);
               service.timeout = "1h";
            }

            Environment environment;
            Server server;
            Executable executable;
            Service service;


            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( environment);
               archive & CASUAL_MAKE_NVP( server);
               archive & CASUAL_MAKE_NVP( executable);
               archive & CASUAL_MAKE_NVP( service);
            )
         };

         struct Domain
         {

            std::string name;
            Default casual_default;
            transaction::Manager transactionmanager;
            std::vector< Group> groups;
            std::vector< Server> servers;
            std::vector< Executable> executables;
            std::vector< Service> services;

            gateway::Gateway gateway;


            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( name);
               archive & sf::makeNameValuePair( "default", casual_default);
               archive & CASUAL_MAKE_NVP( transactionmanager);
               archive & CASUAL_MAKE_NVP( groups);
               archive & CASUAL_MAKE_NVP( servers);
               archive & CASUAL_MAKE_NVP( executables);
               archive & CASUAL_MAKE_NVP( services);
               archive & CASUAL_MAKE_NVP( gateway);

            )

            Domain& operator += ( const Domain& rhs);
            Domain& operator += ( Domain&& rhs);

            friend Domain operator + ( const Domain& lhs, const Domain& rhs);
         };


         Domain get( const std::string& file);
         Domain get( const std::vector< std::string>& files);

         //!
         //! Deserialize the domain configuration
         //!
         //! @return domain configuration
         //!
         Domain get();

         //!
         //! Complement with defaults and validates
         //!
         //! @param configuration domain configuration
         //!
         void finalize( Domain& configuration);


         namespace filter
         {
            struct Membership
            {
               Membership( const std::string& group) : m_group( group) {}

               bool operator () ( const Server& value) const
               {
                  return std::find(
                        std::begin( value.memberships),
                        std::end( value.memberships),
                        m_group) != std::end( value.memberships);
               }
            private:
               std::string m_group;
            };

            struct Excluded
            {
               bool operator () ( const Server& value) const
               {
                  return value.memberships.empty();
               }
            };



         } // filter

      } // domain

   } // config
} // casual

#endif // DOMAIN_H_
