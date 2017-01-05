//!
//! casual
//!

#ifndef CONFIG_DOMAIN_H_
#define CONFIG_DOMAIN_H_


#include "sf/namevaluepair.h"
#include "sf/platform.h"


#include "configuration/server.h"
#include "configuration/service.h"
#include "configuration/group.h"
#include "configuration/environment.h"
#include "configuration/gateway.h"
#include "configuration/queue.h"
#include "configuration/transaction.h"

#include <algorithm>
#include <string>
#include <vector>

namespace casual
{
   namespace configuration
   {
      namespace domain
      {
         namespace domain
         {
            struct Default
            {
               Default();

               Environment environment;

               server::executable::Default server;
               server::executable::Default executable;
               service::service::Default service;


               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( environment);
                  archive & CASUAL_MAKE_NVP( server);
                  archive & CASUAL_MAKE_NVP( executable);
                  archive & CASUAL_MAKE_NVP( service);
               )
            };
         } // domain


         struct Domain
         {

            std::string name;
            domain::Default domain_default;

            std::vector< group::Group> groups;
            std::vector< server::Server> servers;
            std::vector< server::Executable> executables;
            std::vector< service::Service> services;

            transaction::Manager transaction;
            gateway::Manager gateway;
            queue::Manager queue;


            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( name);
               archive & sf::name::value::pair::make( "default", domain_default);
               archive & CASUAL_MAKE_NVP( transaction);
               archive & CASUAL_MAKE_NVP( groups);
               archive & CASUAL_MAKE_NVP( servers);
               archive & CASUAL_MAKE_NVP( executables);
               archive & CASUAL_MAKE_NVP( services);
               archive & CASUAL_MAKE_NVP( gateway);
               archive & CASUAL_MAKE_NVP( queue);

            )

            Domain& operator += ( const Domain& rhs);
            Domain& operator += ( Domain&& rhs);

            friend Domain operator + ( const Domain& lhs, const Domain& rhs);
         };


         Domain get( const std::vector< std::string>& files);


         namespace persistent
         {
            //!
            //! Get the persistent domain configuration
            //!
            //! @return
            //!
            Domain get();

            void save( const Domain& domain);

         } // persistent

         //!
         //! Complement with defaults and validates
         //!
         //! @param configuration domain configuration
         //!
         void finalize( Domain& configuration);

      } // domain

   } // config
} // casual

#endif // DOMAIN_H_
