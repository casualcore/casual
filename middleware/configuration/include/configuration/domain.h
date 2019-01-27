//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "serviceframework/namevaluepair.h"
#include "serviceframework/platform.h"


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
         namespace manager
         {
            struct Default
            {
               Environment environment;

               executable::Default server;
               executable::Default executable;
               service::Default service;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( environment);
                  archive & CASUAL_MAKE_NVP( server);
                  archive & CASUAL_MAKE_NVP( executable);
                  archive & CASUAL_MAKE_NVP( service);
               )

               Default& operator += ( const Default& rhs);
            };

         } // manager


         struct Manager
         {
            std::string name;
            manager::Default manager_default;

            std::vector< Group> groups;
            std::vector< Server> servers;
            std::vector< Executable> executables;
            std::vector< Service> services;

            transaction::Manager transaction;
            gateway::Manager gateway;
            queue::Manager queue;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( name);
               archive & serviceframework::name::value::pair::make( "default", manager_default);
               archive & CASUAL_MAKE_NVP( transaction);
               archive & CASUAL_MAKE_NVP( groups);
               archive & CASUAL_MAKE_NVP( servers);
               archive & CASUAL_MAKE_NVP( executables);
               archive & CASUAL_MAKE_NVP( services);
               archive & CASUAL_MAKE_NVP( gateway);
               archive & CASUAL_MAKE_NVP( queue);
            )

            Manager& operator += ( const Manager& rhs);
            Manager& operator += ( Manager&& rhs);

            friend Manager operator + ( Manager lhs, const Manager& rhs);
         };


         Manager get( const std::vector< std::string>& files);


         //! Complement with defaults and validates
         //!
         //! @param configuration domain configuration
         //!
         void finalize( Manager& configuration);

      } // domain

   } // config
} // casual


