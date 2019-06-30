//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/serialize/macro.h"
#include "common/platform.h"
#include "common/domain.h"


namespace casual
{
   namespace queue
   {
      namespace manager
      {
         namespace admin
         {
            using size_type = common::platform::size::type;


            struct Group
            {
               common::process::Handle process;

               std::string name;
               std::string queuebase;


               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( process);
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( queuebase);
               })
            };
            namespace remote
            {
               struct Domain
               {
                  common::process::Handle process;
                  size_type order = 0;
   
                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( process);
                     CASUAL_SERIALIZE( order);
                  })
               };

               struct Queue
               {
                  std::string name;
                  common::strong::process::id pid;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( name);
                     CASUAL_SERIALIZE( pid);
                  })

                  inline friend bool operator < ( const Queue& lhs, const Queue& rhs)
                  {
                     return std::tie( lhs.name, lhs.pid) 
                          < std::tie( rhs.name, rhs.pid);
                  }
               };
               
            } // remote



            struct Queue
            {
               enum class Type : int
               {
                  group_error_queue = 1,
                  error_queue = 2,
                  queue = 3,
               };


               common::strong::process::id group;
               common::strong::queue::id id;
               std::string name;
               Type type;
               size_type retries;
               common::strong::queue::id error;

               size_type count;
               size_type size;
               size_type uncommitted;
               common::platform::time::point::type timestamp;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( group);
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( type);
                  CASUAL_SERIALIZE( retries);
                  CASUAL_SERIALIZE( error);
                  CASUAL_SERIALIZE( count);
                  CASUAL_SERIALIZE( size);
                  CASUAL_SERIALIZE( uncommitted);
                  CASUAL_SERIALIZE( timestamp);
               })

               inline friend bool operator < ( const Queue& lhs, const Queue& rhs)
               {
                  auto type_order = []( auto& q){ return q.type == Type::group_error_queue ? 1 : 0;};

                  if( type_order( lhs) == type_order( rhs))
                     return lhs.name < rhs.name;

                  return type_order( lhs) < type_order( rhs);
               }
            };

            struct Message
            {
               common::Uuid id;
               common::strong::queue::id queue;
               common::strong::queue::id origin;
               common::platform::binary::type trid;
               size_type state;
               std::string reply;
               size_type redelivered;
               std::string type;

               common::platform::time::point::type available;
               common::platform::time::point::type timestamp;

               size_type size;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( queue);
                  CASUAL_SERIALIZE( origin);
                  CASUAL_SERIALIZE( trid);
                  CASUAL_SERIALIZE( state);
                  CASUAL_SERIALIZE( reply);
                  CASUAL_SERIALIZE( redelivered);
                  CASUAL_SERIALIZE( type);
                  CASUAL_SERIALIZE( available);
                  CASUAL_SERIALIZE( timestamp);
                  CASUAL_SERIALIZE( size);
               })
            };

            struct State
            {
               std::vector< Group> groups;
               std::vector< Queue> queues;

               struct Remote
               {
                  std::vector< remote::Domain> domains;
                  std::vector< remote::Queue> queues;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( domains);
                     CASUAL_SERIALIZE( queues);
                  })

               } remote;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( groups);
                  CASUAL_SERIALIZE( queues);
                  CASUAL_SERIALIZE( remote);
               })

            };


            struct Affected
            {
               struct
               {
                  common::strong::queue::id id;
                  std::string name;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( id);
                     CASUAL_SERIALIZE( name);
                  })
               } queue;


               size_type restored = 0;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( queue);
                  CASUAL_SERIALIZE( restored);
               })
            };


         } // admin
      } // manager
   } // queue

} // casual


