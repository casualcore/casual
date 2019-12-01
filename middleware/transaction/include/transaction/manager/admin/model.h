//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once




#include "common/serialize/macro.h"
#include "casual/platform.h"
#include "common/metric.h"
#include "common/process.h"

namespace casual
{
   namespace transaction
   {
      namespace manager
      {
         namespace admin
         {
            namespace model
            {
               inline namespace v1 {
               struct Metric
               {
                  using time_unit = platform::time::unit;
                  platform::size::type count = 0;
                  time_unit total{};

                  struct Limit 
                  {
                     time_unit min{};
                     time_unit max{};

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( min);
                        CASUAL_SERIALIZE( max);
                     })
                  } limit;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( count);
                     CASUAL_SERIALIZE( total);
                     CASUAL_SERIALIZE( limit);
                  })
               };

               struct Metrics
               {
                  Metric resource;
                  Metric roundtrip;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( resource);
                     CASUAL_SERIALIZE( roundtrip);
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
                        CASUAL_SERIALIZE( id);
                        CASUAL_SERIALIZE( process);
                        CASUAL_SERIALIZE( state);
                        CASUAL_SERIALIZE( metrics);
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
                     platform::size::type concurency = 0;
                     Metrics metrics;

                     std::vector< Instance> instances;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( id);
                        CASUAL_SERIALIZE( name);
                        CASUAL_SERIALIZE( key);
                        CASUAL_SERIALIZE( openinfo);
                        CASUAL_SERIALIZE( closeinfo);
                        CASUAL_SERIALIZE( concurency);
                        CASUAL_SERIALIZE( metrics);
                        CASUAL_SERIALIZE( instances);
                     })

                     inline friend bool operator < ( const Proxy& lhs,  const Proxy& rhs) { return lhs.id < rhs.id;}
                  };
               } // resource

               namespace pending
               {

                  struct Request
                  {
                     resource::id_type resource;
                     common::Uuid correlation;
                     long type;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( resource);
                        CASUAL_SERIALIZE( correlation);
                        CASUAL_SERIALIZE( type);
                     })
                  };

                  struct Reply
                  {
                     std::vector< common::process::Handle> destinations;
                     common::Uuid correlation;
                     long type;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( destinations);
                        CASUAL_SERIALIZE( correlation);
                        CASUAL_SERIALIZE( type);
                     })
                  };

               } // pending

               struct Branch
               {
                  struct ID
                  {
                     common::process::Handle owner;
                     long type;
                     std::string global;
                     std::string branch;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( type);
                        CASUAL_SERIALIZE( owner);
                        CASUAL_SERIALIZE( global);
                        CASUAL_SERIALIZE( branch);
                     })
                  };

                  ID trid;
                  std::vector< resource::id_type> resources;
                  long state{};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( trid);
                     CASUAL_SERIALIZE( resources);
                     CASUAL_SERIALIZE( state);
                  })

               };

               struct Transaction
               {
                  struct Global
                  {
                     std::string id;
                     common::process::Handle owner;
                  
                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( id);
                        CASUAL_SERIALIZE( owner);
                     })
                  };

                  Global global;
                  long state{};
                  std::vector< Branch> branches;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( global);
                     CASUAL_SERIALIZE( branches);
                  })
               };

               struct Log
               {
                  struct update_t
                  {
                     platform::size::type prepare = 0;
                     platform::size::type remove = 0;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( prepare);
                        CASUAL_SERIALIZE( remove);
                     })

                  } update;

                  platform::size::type writes = 0;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( update);
                     CASUAL_SERIALIZE( writes);
                  })
               };

               struct State
               {

                  std::vector< admin::model::resource::Proxy> resources;
                  std::vector< admin::model::Transaction> transactions;

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
                     CASUAL_SERIALIZE( resources);
                     CASUAL_SERIALIZE( transactions);
                     CASUAL_SERIALIZE( persistent.replies);
                     CASUAL_SERIALIZE( persistent.requests);
                     CASUAL_SERIALIZE( pending.requests);
                     CASUAL_SERIALIZE( log);
                  })
               };

               namespace scale
               {
                  struct Instances
                  {
                     std::string name;
                     platform::size::type instances;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( name);
                        CASUAL_SERIALIZE( instances);
                     })

                     inline friend bool operator == ( const Instances& lhs, const Instances& rhs) { return lhs.name == rhs.name;}
                     inline friend bool operator < ( const Instances& lhs, const Instances& rhs) { return lhs.name < rhs.name;}
                  };


               } // scale
            } // inline v1
            } // namespace model
         } // admin
      } // manager
   } // transaction
} // casual


