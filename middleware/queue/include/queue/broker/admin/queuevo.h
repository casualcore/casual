//!
//! brokervo.h
//!
//! Created on: Sep 30, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_QUEUE_BROKER_ADMIN_BROKERVO_H_
#define CASUAL_QUEUE_BROKER_ADMIN_BROKERVO_H_


#include "sf/namevaluepair.h"
#include "sf/platform.h"



namespace casual
{
   namespace queue
   {
      namespace broker
      {
         namespace admin
         {

            struct Group
            {
               struct id_t
               {
                  sf::platform::pid::type pid;
                  sf::platform::ipc::id::type queue;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( pid);
                     archive & CASUAL_MAKE_NVP( queue);
                  })

               } id;
               std::string name;
               std::string queuebase;


               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( id);
                  archive & CASUAL_MAKE_NVP( name);
                  archive & CASUAL_MAKE_NVP( queuebase);
               })
            };

            struct Queue
            {
               sf::platform::pid::type group;
               std::size_t id;
               std::string name;
               std::size_t type;
               std::size_t retries;
               std::size_t error;

               std::size_t count;
               std::size_t size;
               std::size_t uncommitted;
               common::platform::time_point timestamp;

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

               friend bool operator < ( const Queue& lhs, const Queue& rhs)
               {
                  return lhs.id < rhs.id;
               }
            };

            struct Message
            {


               sf::platform::Uuid id;
               std::size_t queue;
               std::size_t origin;
               sf::platform::binary_type trid;
               std::size_t state;
               std::string reply;
               std::size_t redelivered;

               struct buffer_t
               {
                  std::string main;
                  std::string sub;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( main);
                     archive & CASUAL_MAKE_NVP( sub);
                  })
               } type;

               sf::platform::time_point avalible;
               sf::platform::time_point timestamp;

               std::size_t size;


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
                  archive & CASUAL_MAKE_NVP( avalible);
                  archive & CASUAL_MAKE_NVP( timestamp);
                  archive & CASUAL_MAKE_NVP( size);
               })

            };


            struct State
            {
               std::vector< Group> groups;
               std::vector< Queue> queues;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( groups);
                  archive & CASUAL_MAKE_NVP( queues);
               })

            };

            /*
            namespace verbose
            {
               struct GroupVO
               {
                  sf::platform::pid_type pid;
                  sf::platform::ipc::id::type queue_id;
                  std::string name;

                  std::vector< QueueVO> queues;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( pid);
                     archive & CASUAL_MAKE_NVP( name);
                     archive & CASUAL_MAKE_NVP( queues);
                  })

               };
            }
            */


         } // admin
      } // broker
   } // queue

} // casual

#endif // BROKERVO_H_
