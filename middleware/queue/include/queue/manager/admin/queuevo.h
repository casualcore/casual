//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_QUEUE_BROKER_ADMIN_BROKERVO_H_
#define CASUAL_QUEUE_BROKER_ADMIN_BROKERVO_H_


#include "serviceframework/namevaluepair.h"
#include "serviceframework/platform.h"
#include "common/domain.h"



namespace casual
{
   namespace queue
   {
      namespace manager
      {
         namespace admin
         {
            using size_type = serviceframework::platform::size::type;


            struct Group
            {
               common::process::Handle process;

               std::string name;
               std::string queuebase;


               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( process);
                  archive & CASUAL_MAKE_NVP( name);
                  archive & CASUAL_MAKE_NVP( queuebase);
               })
            };
            namespace remote
            {
               struct Domain
               {
                  common::domain::Identity id;
                  common::process::Handle process;
                  size_type order = 0;
   
                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( id);
                     archive & CASUAL_MAKE_NVP( process);
                     archive & CASUAL_MAKE_NVP( order);
                  })
               };

               struct Queue
               {
                  std::string name;
                  common::strong::process::id pid;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( name);
                     archive & CASUAL_MAKE_NVP( pid);
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


               serviceframework::strong::process::id group;
               serviceframework::strong::queue::id id;
               std::string name;
               Type type;
               size_type retries;
               serviceframework::strong::queue::id error;

               size_type count;
               size_type size;
               size_type uncommitted;
               serviceframework::platform::time::point::type timestamp;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( group);
                  archive & CASUAL_MAKE_NVP( id);
                  archive & CASUAL_MAKE_NVP( name);
                  archive & CASUAL_MAKE_NVP( type);
                  archive & CASUAL_MAKE_NVP( retries);
                  archive & CASUAL_MAKE_NVP( error);
                  archive & CASUAL_MAKE_NVP( count);
                  archive & CASUAL_MAKE_NVP( size);
                  archive & CASUAL_MAKE_NVP( uncommitted);
                  archive & CASUAL_MAKE_NVP( timestamp);
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


               serviceframework::platform::Uuid id;
               serviceframework::strong::queue::id queue;
               serviceframework::strong::queue::id origin;
               serviceframework::platform::binary::type trid;
               size_type state;
               std::string reply;
               size_type redelivered;
               std::string type;

               serviceframework::platform::time::point::type available;
               serviceframework::platform::time::point::type timestamp;

               size_type size;


               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( id);
                  archive & CASUAL_MAKE_NVP( queue);
                  archive & CASUAL_MAKE_NVP( origin);
                  archive & CASUAL_MAKE_NVP( trid);
                  archive & CASUAL_MAKE_NVP( state);
                  archive & CASUAL_MAKE_NVP( reply);
                  archive & CASUAL_MAKE_NVP( redelivered);
                  archive & CASUAL_MAKE_NVP( type);
                  archive & CASUAL_MAKE_NVP( available);
                  archive & CASUAL_MAKE_NVP( timestamp);
                  archive & CASUAL_MAKE_NVP( size);
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
                     archive & CASUAL_MAKE_NVP( domains);
                     archive & CASUAL_MAKE_NVP( queues);
                  })

               } remote;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( groups);
                  archive & CASUAL_MAKE_NVP( queues);
                  archive & CASUAL_MAKE_NVP( remote);
               })

            };


            struct Affected
            {
               struct
               {
                  serviceframework::strong::queue::id id;
                  std::string name;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( id);
                     archive & CASUAL_MAKE_NVP( name);
                  })
               } queue;


               size_type restored = 0;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( queue);
                  archive & CASUAL_MAKE_NVP( restored);
               })
            };


         } // admin
      } // manager
   } // queue

} // casual

#endif // BROKERVO_H_
