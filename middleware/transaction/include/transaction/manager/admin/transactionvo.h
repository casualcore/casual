//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once




#include "serviceframework/namevaluepair.h"
#include "serviceframework/platform.h"

namespace casual
{
   namespace transaction
   {
      namespace vo
      {

         struct Statistics
         {

            common::platform::time::unit min = common::platform::time::unit::max();
            common::platform::time::unit max = common::platform::time::unit{ 0};
            common::platform::time::unit total = common::platform::time::unit{ 0};
            serviceframework::platform::size::type invoked = 0;

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
            using id_type = common::strong::resource::id;

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
               common::process::Handle process;

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
               serviceframework::platform::size::type concurency;
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
               serviceframework::platform::Uuid correlation;
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
               serviceframework::strong::ipc::id queue;
               serviceframework::platform::Uuid correlation;
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
               common::process::Handle owner;
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
               serviceframework::platform::size::type prepare = 0;
               serviceframework::platform::size::type remove = 0;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( prepare);
                  archive & CASUAL_MAKE_NVP( remove);
               })

            } update;

            serviceframework::platform::size::type writes = 0;

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
               serviceframework::platform::size::type instances;

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


