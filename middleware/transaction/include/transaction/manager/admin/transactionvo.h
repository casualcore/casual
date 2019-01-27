//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once




#include "serviceframework/namevaluepair.h"
#include "serviceframework/platform.h"
#include "common/metric.h"

namespace casual
{
   namespace transaction
   {
      namespace manager
      {
         namespace admin
         {
            inline namespace v1 
            {
            struct Metric
            {
               using time_unit = serviceframework::platform::time::unit;
               serviceframework::platform::size::type count = 0;
               time_unit total{};

               struct Limit 
               {
                  time_unit min{};
                  time_unit max{};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( min);
                     archive & CASUAL_MAKE_NVP( max);
                  })
               } limit;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( count);
                  archive & CASUAL_MAKE_NVP( total);
                  archive & CASUAL_MAKE_NVP( limit);
               })
            };

            struct Metrics
            {
               Metric resource;
               Metric roundtrip;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( resource);
                  archive & CASUAL_MAKE_NVP( roundtrip);
               })
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

                  Metrics metrics;

                  State state = State::absent;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( id);
                     archive & CASUAL_MAKE_NVP( process);
                     archive & CASUAL_MAKE_NVP( state);
                     archive & CASUAL_MAKE_NVP( metrics);
                  })

                  inline friend bool operator < ( const Instance& lhs,  const Instance& rhs)
                  {
                     if( lhs.id == rhs.id)
                        return lhs.metrics.roundtrip.count > rhs.metrics.roundtrip.count;
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
                  serviceframework::platform::size::type concurency = 0;
                  Metrics metrics;

                  std::vector< Instance> instances;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( id);
                     archive & CASUAL_MAKE_NVP( name);
                     archive & CASUAL_MAKE_NVP( key);
                     archive & CASUAL_MAKE_NVP( openinfo);
                     archive & CASUAL_MAKE_NVP( closeinfo);
                     archive & CASUAL_MAKE_NVP( concurency);
                     archive & CASUAL_MAKE_NVP( metrics);
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

               std::vector< admin::resource::Proxy> resources;
               std::vector< admin::Transaction> transactions;

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
            } // inline v1
         } // admin
      } // manager
   } // transaction
} // casual


