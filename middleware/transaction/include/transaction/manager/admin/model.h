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
#include "common/code/xa.h"
#include "common/message/type.h"

namespace casual
{
   namespace transaction::manager::admin::model
   {
      inline namespace v1 {

      struct Metric
      {
         platform::size::type count = 0;
         platform::time::unit total{};

         struct Limit 
         {
            platform::time::unit min{};
            platform::time::unit max{};

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( min);
               CASUAL_SERIALIZE( max);
            )
         } limit;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( count);
            CASUAL_SERIALIZE( total);
            CASUAL_SERIALIZE( limit);
         )
      };

      struct Metrics
      {
         Metric resource;
         Metric roundtrip;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( resource);
            CASUAL_SERIALIZE( roundtrip);
         )
      };

      namespace resource
      {
         namespace instance
         {
            enum class State : long
            {
               spawned,
               idle,
               busy,
               shutdown
            };

            constexpr std::string_view description( State value)
            {
               switch( value)
               {
                  case State::spawned: return "spawned";
                  case State::idle: return "idle";
                  case State::busy: return "busy";
                  case State::shutdown: return "shutdown";
               }
               return "<unknown>";
            }
            
         } // instance

         struct Instance
         {
            common::strong::resource::id id;
            common::process::Handle process;

            Metrics metrics;
            Metric pending;

            instance::State state{};

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( id);
               CASUAL_SERIALIZE( process);
               CASUAL_SERIALIZE( state);
               CASUAL_SERIALIZE( metrics);
               CASUAL_SERIALIZE( pending);
            )

            inline friend bool operator < ( const Instance& lhs,  const Instance& rhs)
            {
               if( lhs.id == rhs.id)
                  return lhs.metrics.roundtrip.count > rhs.metrics.roundtrip.count;
               return lhs.id < rhs.id;
            }

         };


         struct Proxy
         {
            common::strong::resource::id id;
            std::string name;
            std::string key;
            std::string openinfo;
            std::string closeinfo;
            platform::size::type concurrency = 0;
            Metrics metrics;
            Metric pending;

            std::vector< Instance> instances;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( id);
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( key);
               CASUAL_SERIALIZE( openinfo);
               CASUAL_SERIALIZE( closeinfo);
               CASUAL_SERIALIZE( concurrency);
               CASUAL_SERIALIZE( metrics);
               CASUAL_SERIALIZE( pending);
               CASUAL_SERIALIZE( instances);
            )

            inline friend bool operator == ( const Proxy& lhs, common::strong::resource::id rhs) { return lhs.id == rhs;}

            inline friend bool operator < ( const Proxy& lhs,  const Proxy& rhs) { return lhs.id < rhs.id;}
         };

         namespace external
         {
            struct Instance
            {
               common::strong::resource::id id;
               common::process::Handle process;
               std::string alias;
               std::string description;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( process);
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( description);
               )

               inline friend bool operator == ( const Instance& lhs, common::strong::resource::id rhs) { return lhs.id == rhs;}
               inline friend bool operator < ( const Instance& lhs,  const Proxy& rhs) { return lhs.id < rhs.id;}
            };
         } // external
      } // resource

      namespace pending
      {

         struct Request
         {
            common::strong::resource::id resource;
            common::strong::correlation::id correlation;
            common::message::Type type;
            platform::time::point::type created{};

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( resource);
               CASUAL_SERIALIZE( correlation);
               CASUAL_SERIALIZE( type);
               CASUAL_SERIALIZE( created);
            )
         };

         struct Reply
         {
            common::process::Handle destination;
            common::strong::correlation::id correlation;
            common::message::Type type;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( destination);
               CASUAL_SERIALIZE( correlation);
               CASUAL_SERIALIZE( type);
            )
         };

      } // pending



      namespace transaction
      {
         namespace branch
         {
            struct Resource
            {
               common::strong::resource::id id;
               common::code::xa code = common::code::xa::ok;

               inline friend bool operator == ( const Resource& lhs, common::strong::resource::id rhs) { return lhs.id == rhs;};

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( code);
               )
            };

         } // branch

         struct Branch
         {
            struct ID
            {
               long type;
               std::string global;
               std::string branch;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( type);
                  CASUAL_SERIALIZE( global);
                  CASUAL_SERIALIZE( branch);
               )
            };

            ID trid;
            std::vector< branch::Resource> resources;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( trid);
               CASUAL_SERIALIZE( resources);
            )
         };

         enum struct Stage : short
         {
            involved,
            prepare,
            commit,
            rollback
         };

         inline std::ostream& operator << ( std::ostream& out, Stage value)
         {
            switch( value)
            {
               case Stage::involved: return out << "involved";
               case Stage::prepare: return out << "prepare";
               case Stage::commit: return out << "commit";
               case Stage::rollback: return out << "rollback";
            }
            return out << "<unknown>";
         }

         
      } // transaction

      struct Transaction
      {
         struct Global
         {
            std::string id;
            
            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( id);
            )
         };

         Global global;
         transaction::Stage stage{};
         std::vector< transaction::Branch> branches;
         common::process::Handle owner;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( global);
            CASUAL_SERIALIZE( stage);
            CASUAL_SERIALIZE( branches);
            CASUAL_SERIALIZE( owner);
         )
      };

      struct Log
      {
         struct
         {
            platform::size::type prepare = 0;
            platform::size::type remove = 0;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( prepare);
               CASUAL_SERIALIZE( remove);
            )

         } update;

         platform::size::type writes = 0;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( update);
            CASUAL_SERIALIZE( writes);
         )
      };

      struct State
      {
         std::vector< admin::model::resource::Proxy> resources;
         std::vector< admin::model::resource::external::Instance> externals;
         std::vector< admin::model::Transaction> transactions;

         struct
         {
            struct
            {
               std::vector< pending::Reply> replies;
               std::vector< pending::Request> requests;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( replies);
                  CASUAL_SERIALIZE( requests);
               )
            } persistent;

            std::vector< pending::Request> requests;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( persistent);
               CASUAL_SERIALIZE( requests);
            )

         } pending;

         Log log;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( resources);
            CASUAL_SERIALIZE( externals);
            CASUAL_SERIALIZE( transactions);
            CASUAL_SERIALIZE( pending);
            CASUAL_SERIALIZE( log);
         )
      };

      namespace scale
      {
         struct Instances
         {
            std::string name;
            platform::size::type instances;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( instances);
            )

            inline friend bool operator == ( const Instances& lhs, const Instances& rhs) { return lhs.name == rhs.name;}
            inline friend bool operator < ( const Instances& lhs, const Instances& rhs) { return lhs.name < rhs.name;}
         };


      } // scale
   } // inline v1

   } // transaction::manager::admin::model
} // casual


