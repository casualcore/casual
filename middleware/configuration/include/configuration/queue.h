//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "serviceframework/namevaluepair.h"
#include "serviceframework/platform.h"

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
               serviceframework::optional< serviceframework::platform::size::type> retries;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( retries);
               )
            };
         } // queue

         struct Queue : queue::Default
         {
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
            std::string name;
            serviceframework::optional< std::string> queuebase;
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
            manager::Default manager_default;
            std::vector< Group> groups;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & serviceframework::name::value::pair::make( "default", manager_default);
               archive & CASUAL_MAKE_NVP( groups);
            )

            //! Complement with defaults and validates
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


