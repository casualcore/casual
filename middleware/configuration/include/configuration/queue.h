//!
//! casual
//!

#ifndef CONFIG_QUEUE_H_
#define CONFIG_QUEUE_H_


#include "sf/namevaluepair.h"
#include "sf/platform.h"

#include <string>
#include <vector>

namespace casual
{

   namespace configuration
   {
      namespace queue
      {
         namespace queue
         {
            struct Default
            {
               sf::optional< sf::platform::size::type> retries;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( retries);
               )
            };
         } // queue

         struct Queue : queue::Default
         {
            Queue();
            Queue( std::function<void( Queue&)> foreign);

            std::string name;
            std::string note;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               queue::Default::serialize( archive);
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( note);
            )

            friend bool operator < ( const Queue& lhs, const Queue& rhs);
            friend bool operator == ( const Queue& lhs, const Queue& rhs);
         };

         struct Group
         {
            Group();
            Group( std::function<void( Group&)> foreign);

            std::string name;
            sf::optional< std::string> queuebase;
            std::string note;
            std::vector< Queue> queues;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( queuebase);
               archive & CASUAL_MAKE_NVP( note);
               archive & CASUAL_MAKE_NVP( queues);
            )

            friend bool operator < ( const Group& lhs, const Group& rhs);
            friend bool operator == ( const Group& lhs, const Group& rhs);

         };
         namespace manager
         {
            struct Default
            {
               Default();

               queue::Default queue;
               std::string directory;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( queue);
                  archive & CASUAL_MAKE_NVP( directory);
               )
            };
         } // manager

         struct Manager
         {
            Manager();

            manager::Default manager_default;
            std::vector< Group> groups;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & sf::name::value::pair::make( "default", manager_default);
               archive & CASUAL_MAKE_NVP( groups);
            )

            //!
            //! Complement with defaults and validates
            //!
            void finalize();

            Manager& operator += ( const Manager& rhs);
            Manager& operator += ( Manager&& rhs);
            friend Manager operator + ( const Manager& lhs, const Manager& rhs);
         };




         namespace unittest
         {
            void validate( const Manager& manager);
            void default_values( Manager& manager);
         } // unittest
      } // queue
   } // config
} // casual

#endif // QUEUE_H_
