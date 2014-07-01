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
            std::vector< Queue> queues;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( queues);
            }

         };

         struct Queues
         {
            std::vector< Group> groups;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( groups);
            }
         };


         Queues get( const std::string& file);

         Queues get();

      } // queue

   } // config


} // casual

#endif // QUEUE_H_
