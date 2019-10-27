//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/serialize/macro.h"
#include "common/platform.h"
#include "common/optional.h"

#include <string>
#include <vector>

namespace casual
{
   namespace configuration
   {
      namespace queue
      {
         struct Queue
         {
            struct Retry 
            {
               common::optional< common::platform::size::type> count;
               common::optional< std::string> delay;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( count);
                  CASUAL_SERIALIZE( delay);
               )
            };

            struct Default
            {
               common::optional< Retry> retry;
            
               //! @deprecated
               common::optional< common::platform::size::type> retries;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( retry);

                  CASUAL_SERIALIZE( retries);
               )
            };

            std::string name;
            common::optional< Retry> retry;
            common::optional< std::string> note;

            //! @deprecated
            common::optional< common::platform::size::type> retries;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( retry);
               CASUAL_SERIALIZE( note);

               CASUAL_SERIALIZE( retries);
            )

            Queue& operator += ( const Default& value);
            friend bool operator < ( const Queue& lhs, const Queue& rhs);
            friend bool operator == ( const Queue& lhs, const Queue& rhs);
         };

         struct Group
         {
            std::string name;
            common::optional< std::string> queuebase;
            common::optional< std::string> note;
            std::vector< Queue> queues;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( queuebase);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( queues);
            )

            friend bool operator < ( const Group& lhs, const Group& rhs);
            friend bool operator == ( const Group& lhs, const Group& rhs);

         };

         struct Manager
         {
            struct Default
            {
               Default();

               Queue::Default queue;
               std::string directory;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( queue);
                  CASUAL_SERIALIZE( directory);
               )
            };

         
            Default manager_default;
            std::vector< Group> groups;

            common::optional< std::string> note;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE_NAME( manager_default, "default");
               CASUAL_SERIALIZE( groups);
               CASUAL_SERIALIZE( note);
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


