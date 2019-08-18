//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/serialize/macro.h"
#include "common/platform.h"


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
         inline namespace v1 {
            
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
                  CASUAL_SERIALIZE( environment);
                  CASUAL_SERIALIZE( server);
                  CASUAL_SERIALIZE( executable);
                  CASUAL_SERIALIZE( service);
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
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE_NAME( manager_default, "default");
               CASUAL_SERIALIZE( transaction);
               CASUAL_SERIALIZE( groups);
               CASUAL_SERIALIZE( servers);
               CASUAL_SERIALIZE( executables);
               CASUAL_SERIALIZE( services);
               CASUAL_SERIALIZE( gateway);
               CASUAL_SERIALIZE( queue);

            )

            Manager& operator += ( const Manager& rhs);
            Manager& operator += ( Manager&& rhs);

            friend Manager operator + ( Manager lhs, const Manager& rhs);
         };


         Manager get( const std::vector< std::string>& files);

         //! Complement with defaults and validates
         //!
         //! @param configuration domain configuration
         void finalize( Manager& configuration);

      } // v1
      } // domain

   } // config
} // casual


