//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "serviceframework/namevaluepair.h"
#include "serviceframework/platform.h"

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
               serviceframework::optional< serviceframework::platform::size::type> size;
               serviceframework::optional< serviceframework::platform::size::type> messages;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( size);
                  archive & CASUAL_MAKE_NVP( messages);
               )
            };

            struct Default
            {
               Limit limit;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( limit);
               )
            };

         } // listener

         struct Listener
         {
            std::string address;
            serviceframework::optional< listener::Limit> limit;
            std::string note;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( address);
               archive & CASUAL_MAKE_NVP( limit);
               archive & CASUAL_MAKE_NVP( note);
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
                  archive & CASUAL_MAKE_NVP( restart);
                  archive & CASUAL_MAKE_NVP( address);
               )
            };
         } // connection

         struct Connection
         {
            serviceframework::optional< std::string> address;
            std::vector< std::string> services;
            std::vector< std::string> queues;
            std::string note;
            serviceframework::optional< bool> restart;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( address);
               archive & CASUAL_MAKE_NVP( services);
               archive & CASUAL_MAKE_NVP( queues);
               archive & CASUAL_MAKE_NVP( restart);
               archive & CASUAL_MAKE_NVP( note);
            )

            Connection& operator += ( const connection::Default& rhs);
            friend bool operator == ( const Connection& lhs, const Connection& rhs);
            
         };

         namespace manager
         {
            struct Default
            {
               listener::Default listener;
               connection::Default connection;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( listener);
                  archive & CASUAL_MAKE_NVP( connection);
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
               archive & serviceframework::name::value::pair::make( "default", manager_default);
               archive & CASUAL_MAKE_NVP( listeners);
               archive & CASUAL_MAKE_NVP( connections);
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


