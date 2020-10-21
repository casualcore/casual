//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/serialize/macro.h"
#include "casual/platform.h"
#include <optional>

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
               std::optional< platform::size::type> count;
               std::optional< std::string> delay;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( count);
                  CASUAL_SERIALIZE( delay);
               )
            };

            struct Default
            {
               std::optional< Retry> retry;
            
               //! @deprecated
               std::optional< platform::size::type> retries;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( retry);

                  CASUAL_SERIALIZE( retries);
               )
            };

            std::string name;
            std::optional< Retry> retry;
            std::optional< std::string> note;

            //! @deprecated
            std::optional< platform::size::type> retries;

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
            std::optional< std::string> queuebase;
            std::optional< std::string> note;
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

         struct Forward
         {
            struct Default
            {
               struct
               {
                  platform::size::type instances = 1;

                  struct
                  {
                     std::optional< std::string> delay;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        CASUAL_SERIALIZE( delay);
                     )
                  } reply;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( instances);
                     CASUAL_SERIALIZE( reply);
                  )

               } service;

               struct
               {
                  platform::size::type instances = 1;

                  struct
                  {
                     std::optional< std::string> delay;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        CASUAL_SERIALIZE( delay);
                     )
                  } target;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( instances);
                     CASUAL_SERIALIZE( target);
                  )
               } queue;


               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( service);
                  CASUAL_SERIALIZE( queue);
               )
            };

            struct Source
            {
               std::string name;
               
               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( name);
               )
            };

            struct forward_base
            {
               std::string alias;
               Source source;
               std::optional< platform::size::type> instances;
               std::optional< std::string> note;

               inline friend bool operator == ( const forward_base& lhs, const forward_base& rhs) { return lhs.alias == rhs.alias;}

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( source);
                  CASUAL_SERIALIZE( instances);
                  CASUAL_SERIALIZE( note);
               )
            };

            struct Queue : forward_base
            {
               struct Target
               {
                  std::string name;
                  std::optional< std::string> delay;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( name);
                     CASUAL_SERIALIZE( delay);
                  )
               };

               Target target;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  forward_base::serialize( archive);
                  CASUAL_SERIALIZE( target);
               )
            };

            struct Service : forward_base
            {
               using Target = Source;
               using Reply = Queue::Target;

               Target target;
               std::optional< Reply> reply;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  forward_base::serialize( archive);
                  CASUAL_SERIALIZE( target);
                  CASUAL_SERIALIZE( reply);
               )
            };

            std::vector< Service> services;
            std::vector< Queue> queues;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE( services);
               CASUAL_SERIALIZE( queues);
            )
         };

         struct Manager
         {
            struct Default
            {
               Default();

               Queue::Default queue;
               Forward::Default forward;
               std::string directory;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( queue);
                  CASUAL_SERIALIZE( forward);
                  CASUAL_SERIALIZE( directory);
               )
            };

         
            Default manager_default;
            std::vector< Group> groups;
            Forward forward;

            std::optional< std::string> note;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE_NAME( manager_default, "default");
               CASUAL_SERIALIZE( groups);
               CASUAL_SERIALIZE( forward);
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


