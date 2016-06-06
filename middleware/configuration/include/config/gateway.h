//!
//! casual
//!

#ifndef CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_GATEWAY_H_
#define CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_GATEWAY_H_


#include "common/message/domain.h"

#include "sf/namevaluepair.h"

#include <string>
#include <vector>

namespace casual
{
   namespace config
   {
      namespace gateway
      {

         struct Listener
         {
            std::string address;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( address);
            )

            friend bool operator == ( const Listener& lhs, const Listener& rhs);
         };


         struct Connection
         {
            std::string name;
            std::string type;
            std::string address;
            std::string restart;
            std::vector< std::string> services;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( type);
               archive & CASUAL_MAKE_NVP( address);
               archive & CASUAL_MAKE_NVP( restart);
               archive & CASUAL_MAKE_NVP( services);
            )

            friend bool operator == ( const Connection& lhs, const Connection& rhs);
         };

         struct Default
         {
            Default()
            {
               connection.type = "tcp";
               connection.restart = "false";
            }

            Listener listener;
            Connection connection;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( listener);
               archive & CASUAL_MAKE_NVP( connection);
            )
         };

         struct Gateway
         {
            Default casual_default;
            std::vector< gateway::Listener> listeners;
            std::vector< gateway::Connection> connections;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & sf::makeNameValuePair( "default", casual_default);
               archive & CASUAL_MAKE_NVP( listeners);
               archive & CASUAL_MAKE_NVP( connections);
            )

            //!
            //! Complement with defaults and validates
            //!
            void finalize();

            Gateway& operator += ( const Gateway& rhs);
            Gateway& operator += ( Gateway&& rhs);
            friend Gateway operator + ( const Gateway& lhs, const Gateway& rhs);

         };




         Gateway get( const std::string& file);

         Gateway get();


         namespace transform
         {
            common::message::domain::configuration::gateway::Reply gateway( const Gateway& value);

         } // transform

      } // gateway
   } // config
} // casual

#endif // CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_GATEWAY_H_
