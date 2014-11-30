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

            struct GroupVO
            {
               sf::platform::pid_type pid;
               sf::platform::queue_id_type queue_id;
               std::string name;

               std::vector< std::string> queues;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( pid);
                  archive & CASUAL_MAKE_NVP( name);
                  archive & CASUAL_MAKE_NVP( queues);
               })
            };

            struct QueueVO
            {
               std::size_t id;
               std::string name;
               std::size_t type;
               std::size_t retries;
               std::size_t error;
               std::size_t messages;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( id);
                  archive & CASUAL_MAKE_NVP( name);
                  archive & CASUAL_MAKE_NVP( type);
                  archive & CASUAL_MAKE_NVP( retries);
                  archive & CASUAL_MAKE_NVP( error);
                  archive & CASUAL_MAKE_NVP( messages);
               })

               friend bool operator < ( const QueueVO& lhs, const QueueVO& rhs)
               {
                  return lhs.id < rhs.id;
               }
            };


            namespace verbose
            {
               struct GroupVO
               {
                  sf::platform::pid_type pid;
                  sf::platform::queue_id_type queue_id;
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


         } // admin
      } // broker
   } // queue

} // casual

#endif // BROKERVO_H_
