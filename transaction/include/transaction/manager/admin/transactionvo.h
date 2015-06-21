//!
//! transactionvo.h
//!
//! Created on: Jun 14, 2015
//!     Author: Lazan
//!

#ifndef CASUAL_TRANSACTION_MANAGER_ADMIN_TRANSACTIONVO_H_
#define CASUAL_TRANSACTION_MANAGER_ADMIN_TRANSACTIONVO_H_



#include "sf/namevaluepair.h"
#include "sf/platform.h"

namespace casual
{
   namespace transaction
   {
      namespace vo
      {
         struct Process
         {
            sf::platform::pid_type pid;
            sf::platform::queue_id_type queue;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( pid);
               archive & CASUAL_MAKE_NVP( queue);
            })
         };

         namespace resource
         {
            using id_type = common::platform::resource::id_type;

            struct Instance
            {
               enum class State : long
               {
                  absent,
                  started,
                  idle,
                  busy,
                  startupError,
                  shutdown
               };

               id_type id;
               Process process;
               State state = State::absent;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( id);
                  archive & CASUAL_MAKE_NVP( process);
               })
            };


            struct Proxy
            {
               id_type id;
               std::string key;
               std::string openinfo;
               std::string closeinfo;
               std::size_t concurency;

               std::vector< Instance> instances;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( id);
                  archive & CASUAL_MAKE_NVP( key);
                  archive & CASUAL_MAKE_NVP( openinfo);
                  archive & CASUAL_MAKE_NVP( closeinfo);
                  archive & CASUAL_MAKE_NVP( concurency);
                  archive & CASUAL_MAKE_NVP( instances);
               })
            };
         } // resource

         namespace pending
         {

            struct Request
            {
               std::vector< resource::id_type> resources;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( resources);
               })
            };

            struct Reply
            {
               sf::platform::queue_id_type queue;
               sf::platform::Uuid correlation;
               long type;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( queue);
                  archive & CASUAL_MAKE_NVP( correlation);
                  archive & CASUAL_MAKE_NVP( type);
               })
            };

         } // pending

         struct Transaction
         {
            struct ID
            {
               Process owner;
               long type;
               std::string global;
               std::string branch;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( type);
                  archive & CASUAL_MAKE_NVP( owner);
                  archive & CASUAL_MAKE_NVP( global);
                  archive & CASUAL_MAKE_NVP( branch);
               })
            };

            ID trid;
            std::vector< resource::id_type> resources;
            long state;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( trid);
               archive & CASUAL_MAKE_NVP( resources);
               archive & CASUAL_MAKE_NVP( state);
            })

         };


         struct State
         {

            std::vector< vo::resource::Proxy> resources;
            std::vector< vo::Transaction> transactions;

            struct persistent_t
            {
               std::vector< pending::Reply> replies;
               std::vector< pending::Request> requests;
            } persistent;

            struct pending_t
            {
               std::vector< pending::Request> requests;
            } pending;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( resources);
               archive & CASUAL_MAKE_NVP( transactions);
               archive & CASUAL_MAKE_NVP( persistent.replies);
               archive & CASUAL_MAKE_NVP( persistent.requests);
               archive & CASUAL_MAKE_NVP( pending.requests);
            })
          };


      } // vo


   } // transaction



} // casual

#endif // TRANSACTIONVO_H_
