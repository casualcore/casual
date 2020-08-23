//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/serialize/macro.h"
#include "casual/platform.h"
#include "common/optional.h"

#include <string>
#include <vector>

namespace casual
{
   namespace configuration
   {
      namespace gateway
      {
         namespace listener
         {
            struct Limit
            {
               common::optional< platform::size::type> size;
               common::optional< platform::size::type> messages;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( size);
                  CASUAL_SERIALIZE( messages);
               )
            };

            struct Default
            {
               Limit limit;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( limit);
               )
            };

         } // listener

         struct Listener
         {
            std::string address;
            common::optional< listener::Limit> limit;
            std::string note;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE( address);
               CASUAL_SERIALIZE( limit);
               CASUAL_SERIALIZE( note);
            )

            Listener& operator += ( const listener::Default& rhs);
            friend bool operator == ( const Listener& lhs, const Listener& rhs);
         };

         namespace connection
         {
            struct Default
            {
               bool restart = true;

               // TODO: why address in default?
               std::string address;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( restart);
                  CASUAL_SERIALIZE( address);
               )
            };
         } // connection

         struct Connection
         {
            common::optional< std::string> address;
            std::vector< std::string> services;
            std::vector< std::string> queues;
            std::string note;
            common::optional< bool> restart;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE( address);
               CASUAL_SERIALIZE( services);
               CASUAL_SERIALIZE( queues);
               CASUAL_SERIALIZE( restart);
               CASUAL_SERIALIZE( note);
            )

            Connection& operator += ( const connection::Default& rhs);            
         };

         namespace manager
         {
            struct Default
            {
               listener::Default listener;
               connection::Default connection;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( listener);
                  CASUAL_SERIALIZE( connection);
               )
            };
         } // manager


         struct Manager
         {
            manager::Default manager_default;
            std::vector< gateway::Listener> listeners;
            std::vector< gateway::Connection> connections;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE_NAME( manager_default, "default");
               CASUAL_SERIALIZE( listeners);
               CASUAL_SERIALIZE( connections);
            )

            //! Complement with defaults and validates
            void finalize();

            Manager& operator += ( const Manager& rhs);
            Manager& operator += ( Manager&& rhs);
            friend Manager operator + ( Manager lhs, const Manager& rhs);

         };


      } // gateway
   } // configuration
} // casual


