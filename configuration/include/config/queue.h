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
            std::string retries;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( retries);
            }

            friend bool operator < ( const Queue& lhs, const Queue& rhs);
            friend bool operator == ( const Queue& lhs, const Queue& rhs);
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

            friend bool operator < ( const Group& lhs, const Group& rhs);
            friend bool operator == ( const Group& lhs, const Group& rhs);

         };

         struct Default
         {
            std::string path;
            Queue queue;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( path);
               archive & CASUAL_MAKE_NVP( queue);
            }
         };

         struct Domain
         {
            Default casual_default;
            std::vector< Group> groups;

            template< typename A>
            void serialize( A& archive)
            {
               archive & sf::makeNameValuePair( "default", casual_default);
               archive & CASUAL_MAKE_NVP( groups);
            }
         };


         Domain get( const std::string& file);

         Domain get();

         namespace unittest
         {
            void validate( const Domain& domain);
            void default_values( Domain& domain);
         } // unittest

      } // queue

   } // config


} // casual

#endif // QUEUE_H_
