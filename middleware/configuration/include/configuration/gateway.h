//!
//! casual
//!

#ifndef CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_GATEWAY_H_
#define CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_GATEWAY_H_


#include "sf/namevaluepair.h"
#include "sf/platform.h"

#include <string>
#include <vector>

namespace casual
{
   namespace configuration
   {
      namespace gateway
      {

         struct Listener
         {
            Listener() = default;
            Listener( std::function<void( Listener&)> foreign) { foreign( *this);}

            std::string address;
            std::string note;


            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( address);
               archive & CASUAL_MAKE_NVP( note);
            )

            friend bool operator == ( const Listener& lhs, const Listener& rhs);
         };

         namespace connection
         {
            struct Default
            {
               sf::optional< std::string> type;
               sf::optional< bool> restart;
               sf::optional< std::string> address;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( type);
                  archive & CASUAL_MAKE_NVP( restart);
                  archive & CASUAL_MAKE_NVP( address);
               )

            };

         } // connection

         struct Connection : connection::Default
         {
            Connection() = default;
            Connection( std::function<void( Connection&)> foreign) { foreign( *this);}

            std::vector< std::string> services;
            std::vector< std::string> queues;
            std::string note;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               connection::Default::serialize( archive);
               archive & CASUAL_MAKE_NVP( note);
               archive & CASUAL_MAKE_NVP( services);
               archive & CASUAL_MAKE_NVP( queues);
            )

            friend bool operator == ( const Connection& lhs, const Connection& rhs);
            friend Connection& operator += ( Connection& lhs, const connection::Default& rhs);
         };

         namespace manager
         {
            struct Default
            {
               Default();

               connection::Default connection;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
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
               archive & sf::name::value::pair::make( "default", manager_default);
               archive & CASUAL_MAKE_NVP( listeners);
               archive & CASUAL_MAKE_NVP( connections);
            )

            //!
            //! Complement with defaults and validates
            //!
            void finalize();

            Manager& operator += ( const Manager& rhs);
            Manager& operator += ( Manager&& rhs);
            friend Manager operator + ( const Manager& lhs, const Manager& rhs);

         };


      } // gateway
   } // configuration
} // casual

#endif // CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_GATEWAY_H_
