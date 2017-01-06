//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_MESSAGE_H_
#define CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_MESSAGE_H_

#include "common/marshal/marshal.h"
#include "common/message/type.h"

#include <string>
#include <vector>

namespace casual
{

   namespace configuration
   {
      namespace message
      {
         struct Environment
         {
            struct Variable
            {
               std::string key;
               std::string value;

               CASUAL_CONST_CORRECT_MARSHAL(
                  archive & key;
                  archive & value;
               )
            };

            std::vector< Variable> variables;

            CASUAL_CONST_CORRECT_MARSHAL(
               archive & variables;
            )
         };

         namespace transaction
         {
            struct Resource
            {
               Resource() = default;
               inline Resource( std::function< void(Resource&)> foreign) { foreign( *this);}

               std::string name;
               std::string key;
               std::size_t instances = 0;
               std::string note;

               std::string openinfo;
               std::string closeinfo;

               CASUAL_CONST_CORRECT_MARSHAL(
                  archive & name;
                  archive & key;
                  archive & instances;
                  archive & note;
                  archive & openinfo;
                  archive & closeinfo;
               )
            };

         } // transaction

         struct Transaction
         {
            std::string log;
            std::vector< transaction::Resource> resources;

            CASUAL_CONST_CORRECT_MARSHAL
            (
               archive & log;
               archive & resources;
            )
         };

         namespace gateway
         {
            struct Listener
            {
               std::string address;

               CASUAL_CONST_CORRECT_MARSHAL
               (
                  archive & address;
               )
            };

            struct Connection
            {
               enum class Type : char
               {
                  ipc,
                  tcp
               };
               Type type = Type::tcp;
               bool restart = true;
               std::string address;
               std::string note;
               std::vector< std::string> services;
               std::vector< std::string> queues;

               CASUAL_CONST_CORRECT_MARSHAL
               (
                  archive & type;
                  archive & restart;
                  archive & address;
                  archive & note;
                  archive & services;
                  archive & queues;
               )
            };
         } // gateway

         struct Gateway
         {
            std::vector< gateway::Listener> listeners;
            std::vector< gateway::Connection> connections;

            CASUAL_CONST_CORRECT_MARSHAL
            (
               archive & listeners;
               archive & connections;
            )
         };

         namespace queue
         {
            struct Queue
            {
               std::string name;
               std::size_t retries = 0;
               std::string note;

               CASUAL_CONST_CORRECT_MARSHAL
               (
                  archive & name;
                  archive & retries;
                  archive & note;
               )
            };

            struct Group
            {
               std::string name;
               std::string queuebase;
               std::string note;
               std::vector< Queue> queues;

               CASUAL_CONST_CORRECT_MARSHAL
               (
                  archive & name;
                  archive & queuebase;
                  archive & note;
                  archive & queues;
               )
            };

            struct Manager
            {
               std::vector< Group> groups;

               CASUAL_CONST_CORRECT_MARSHAL
               (
                  archive & groups;
               )
            };

         } // queue

         struct Domain
         {
            std::string name;

            Transaction transaction;
            Gateway gateway;
            queue::Manager queue;


            CASUAL_CONST_CORRECT_MARSHAL
            (
               archive & name;
               archive & transaction;
               archive & gateway;
               archive & queue;
            )
         };


         struct Request : common::message::basic_request< common::message::Type::configuration_request>
         {

         };

         using base_reply = common::message::basic_reply< common::message::Type::configuration_reply>;
         struct Reply : base_reply
         {
            Domain domain;

            CASUAL_CONST_CORRECT_MARSHAL(
               base_reply::marshal( archive);
               archive & domain;
            )

         };

      } // message

   } // configuration

   namespace common
   {
      namespace message
      {
         namespace reverse
         {
            template<>
            struct type_traits< configuration::message::Request> : detail::type< configuration::message::Reply> {};
         } // reverse
      } // message
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_INTERNAL_CONFIGURATION_H_
