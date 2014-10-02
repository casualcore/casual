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
               std::string name;

               std::vector< std::string> queues;

               template< typename A>
               void serialize( A& archive)
               {
                  archive & CASUAL_MAKE_NVP( pid);
                  archive & CASUAL_MAKE_NVP( name);
               }
            };

            struct QueueVO
            {

            };


         } // admin
      } // broker
   } // queue

} // casual

#endif // BROKERVO_H_
