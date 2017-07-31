//!
//! casual
//!

#ifndef CASUAL_QUEUE_BROKER_ADMIN_BROKERVO_H_
#define CASUAL_QUEUE_BROKER_ADMIN_BROKERVO_H_


#include "sf/namevaluepair.h"
#include "sf/platform.h"



namespace casual
{
   namespace queue
   {
      namespace manager
      {
         namespace admin
         {
            using size_type = sf::platform::size::type;


            struct Group
            {
               struct
               {
                  sf::platform::pid::type pid;
                  sf::platform::ipc::handle::type queue;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( pid);
                     archive & CASUAL_MAKE_NVP( queue);
                  })
               } process;

               std::string name;
               std::string queuebase;


               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( process);
                  archive & CASUAL_MAKE_NVP( name);
                  archive & CASUAL_MAKE_NVP( queuebase);
               })
            };

            struct Queue
            {
               enum class Type : int
               {
                  group_error_queue = 1,
                  error_queue = 2,
                  queue = 3,
               };


               sf::platform::pid::type group;
               size_type id;
               std::string name;
               Type type;
               size_type retries;
               size_type error;

               size_type count;
               size_type size;
               size_type uncommitted;
               sf::platform::time::point::type timestamp;

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
               size_type queue;
               size_type origin;
               sf::platform::binary::type trid;
               size_type state;
               std::string reply;
               size_type redelivered;
               std::string type;

               sf::platform::time::point::type available;
               sf::platform::time::point::type timestamp;

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

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( groups);
                  archive & CASUAL_MAKE_NVP( queues);
               })

            };


            struct Affected
            {
               struct
               {
                  size_type id;
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
