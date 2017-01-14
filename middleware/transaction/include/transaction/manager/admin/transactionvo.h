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
            sf::platform::pid::type pid;
            sf::platform::ipc::id::type queue;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( pid);
               archive & CASUAL_MAKE_NVP( queue);
            })
         };

         struct Statistics
         {

            std::chrono::microseconds min = std::chrono::microseconds::max();
            std::chrono::microseconds max = std::chrono::microseconds{ 0};
            std::chrono::microseconds total = std::chrono::microseconds{ 0};
            std::size_t invoked = 0;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( min);
               archive & CASUAL_MAKE_NVP( max);
               archive & CASUAL_MAKE_NVP( total);
               archive & CASUAL_MAKE_NVP( invoked);
            })

            inline friend Statistics& operator += ( Statistics& lhs, const Statistics& rhs)
            {
               if( lhs.min > rhs.min) lhs.min = rhs.min;
               if( lhs.max < rhs.max) lhs.max = rhs.max;
               lhs.total += rhs.total;
               lhs.invoked += rhs.invoked;

               return lhs;
            }
         };

         struct Stats
         {

            Statistics resource;
            Statistics roundtrip;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( resource);
               archive & CASUAL_MAKE_NVP( roundtrip);
            })

            inline friend Stats& operator += ( Stats& lhs, const Stats& rhs)
            {
               lhs.resource += rhs.resource;
               lhs.roundtrip += rhs.roundtrip;

               return lhs;
            }
         };

         namespace resource
         {
            using id_type = common::platform::resource::id::type;

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

               Stats statistics;

               State state = State::absent;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( id);
                  archive & CASUAL_MAKE_NVP( process);
                  archive & CASUAL_MAKE_NVP( state);
                  archive & CASUAL_MAKE_NVP( statistics);
               })

               inline friend bool operator < ( const Instance& lhs,  const Instance& rhs)
               {
                  if( lhs.id == rhs.id)
                     return lhs.statistics.roundtrip.invoked > rhs.statistics.roundtrip.invoked;
                  return lhs.id < rhs.id;
               }

            };


            struct Proxy
            {
               id_type id;
               std::string name;
               std::string key;
               std::string openinfo;
               std::string closeinfo;
               std::size_t concurency;
               Stats statistics;

               std::vector< Instance> instances;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( id);
                  archive & CASUAL_MAKE_NVP( name);
                  archive & CASUAL_MAKE_NVP( key);
                  archive & CASUAL_MAKE_NVP( openinfo);
                  archive & CASUAL_MAKE_NVP( closeinfo);
                  archive & CASUAL_MAKE_NVP( concurency);
                  archive & CASUAL_MAKE_NVP( statistics);
                  archive & CASUAL_MAKE_NVP( instances);
               })

               inline friend bool operator < ( const Proxy& lhs,  const Proxy& rhs) { return lhs.id < rhs.id;}
            };
         } // resource

         namespace pending
         {

            struct Request
            {
               resource::id_type resource;
               sf::platform::Uuid correlation;
               long type;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( resource);
                  archive & CASUAL_MAKE_NVP( correlation);
                  archive & CASUAL_MAKE_NVP( type);
               })
            };

            struct Reply
            {
               sf::platform::ipc::id::type queue;
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

         struct Log
         {
            struct update_t
            {
               std::size_t prepare = 0;
               std::size_t remove = 0;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( prepare);
                  archive & CASUAL_MAKE_NVP( remove);
               })

            } update;

            std::size_t writes = 0;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( update);
               archive & CASUAL_MAKE_NVP( writes);
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

            Log log;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( resources);
               archive & CASUAL_MAKE_NVP( transactions);
               archive & CASUAL_MAKE_NVP( persistent.replies);
               archive & CASUAL_MAKE_NVP( persistent.requests);
               archive & CASUAL_MAKE_NVP( pending.requests);
               archive & CASUAL_MAKE_NVP( log);
            })
          };

         namespace update
         {
            struct Instances
            {
               resource::id_type id;
               std::size_t instances;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( id);
                  archive & CASUAL_MAKE_NVP( instances);
               })

               inline friend bool operator == ( const Instances& lhs, const Instances& rhs) { return lhs.id == rhs.id;}
               inline friend bool operator < ( const Instances& lhs, const Instances& rhs) { return lhs.id < rhs.id;}
            };


         } // update

      } // vo


   } // transaction



} // casual

#endif // TRANSACTIONVO_H_
