//!
//! queue.h
//!
//! Created on: Jun 30, 2014
//!     Author: Lazan
//!

#ifndef CONFIG_QUEUE_H_
#define CONFIG_QUEUE_H_


#include "sf/namevaluepair.h"

#include <string>
#include <vector>

namespace casual
{

   namespace config
   {
      namespace queue
      {

         struct Queue
         {
            std::string name;
            std::size_t retries = 0;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( retries);
            }
         };

         struct Group
         {
            std::string name;
            std::string queuebase;
            std::vector< Queue> queues;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( queuebase);
               archive & CASUAL_MAKE_NVP( queues);
            }

         };

         struct Domain
         {
            std::vector< Group> groups;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( groups);
            }
         };


         Domain get( const std::string& file);

         Domain get();

      } // queue

   } // config


} // casual

#endif // QUEUE_H_
