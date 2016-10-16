//!
//! casual
//!

#ifndef CONFIG_QUEUE_H_
#define CONFIG_QUEUE_H_


#include "sf/namevaluepair.h"

#include "common/message/domain.h"

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
            std::string note;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( retries);
               archive & CASUAL_MAKE_NVP( note);
            }

            friend bool operator < ( const Queue& lhs, const Queue& rhs);
            friend bool operator == ( const Queue& lhs, const Queue& rhs);
         };

         struct Group
         {
            std::string name;
            std::string queuebase;
            std::string note;
            std::vector< Queue> queues;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( queuebase);
               archive & CASUAL_MAKE_NVP( note);
               archive & CASUAL_MAKE_NVP( queues);
            }

            friend bool operator < ( const Group& lhs, const Group& rhs);
            friend bool operator == ( const Group& lhs, const Group& rhs);

         };

         struct Default
         {
            Default();

            Queue queue;

            template< typename A>
            void serialize( A& archive)
            {
               archive & CASUAL_MAKE_NVP( queue);
            }
         };

         struct Manager
         {
            Default casual_default;
            std::vector< Group> groups;

            template< typename A>
            void serialize( A& archive)
            {
               archive & sf::makeNameValuePair( "default", casual_default);
               archive & CASUAL_MAKE_NVP( groups);
            }

            //!
            //! Complement with defaults and validates
            //!
            void finalize();

            Manager& operator += ( const Manager& rhs);
            Manager& operator += ( Manager&& rhs);
            friend Manager operator + ( const Manager& lhs, const Manager& rhs);
         };



         namespace transform
         {
            common::message::domain::configuration::queue::Reply manager( const Manager& value);

         } // transform


         namespace unittest
         {
            void validate( const Manager& manager);
            void default_values( Manager& manager);
         } // unittest
      } // queue
   } // config
} // casual

#endif // QUEUE_H_
