//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/serialize/macro.h"
#include "casual/platform.h"
#include "common/domain.h"

#include <algorithm>


namespace casual
{
   namespace queue::manager::admin::model
   {
      inline namespace v1 {
      
      struct Group
      {
         std::string name;
         common::process::Handle process;
         std::string queuebase;
         std::string note;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( process);
            CASUAL_SERIALIZE( name);
            CASUAL_SERIALIZE( queuebase);
            CASUAL_SERIALIZE( note);
         )
      };

      namespace remote
      {
         struct Domain
         {
            common::process::Handle process;
            platform::size::type order = 0;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( process);
               CASUAL_SERIALIZE( order);
            )
         };

         struct Queue
         {
            std::string name;
            common::strong::process::id pid;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( pid);
            )

            inline friend bool operator < ( const Queue& lhs, const Queue& rhs)
            {
               return std::tie( lhs.name, lhs.pid) 
                  < std::tie( rhs.name, rhs.pid);
            }
         };
         
      } // remote



      struct Queue
      {
         enum class Type : int
         {
            queue = 1,
            error_queue = 2,
         };

         struct Retry 
         {
            platform::size::type count = 0;
            platform::time::unit delay{};

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( count);
               CASUAL_SERIALIZE( delay);
            )
         };

         inline Type type() const { return  error ? Type::queue : Type::error_queue;}

         common::strong::process::id group;
         common::strong::queue::id id;
         std::string name;
         Retry retry;
         common::strong::queue::id error;

         platform::size::type count{};
         platform::size::type size{};
         platform::size::type uncommitted{};

         struct
         {
            platform::size::type dequeued{};
            platform::size::type enqueued{};

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( dequeued);
               CASUAL_SERIALIZE( enqueued);
            )

         } metric;

         platform::time::point::type last;
         platform::time::point::type created;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( group);
            CASUAL_SERIALIZE( id);
            CASUAL_SERIALIZE( name);
            CASUAL_SERIALIZE( retry);
            CASUAL_SERIALIZE( error);
            CASUAL_SERIALIZE( count);
            CASUAL_SERIALIZE( size);
            CASUAL_SERIALIZE( uncommitted);
            CASUAL_SERIALIZE( metric);
            CASUAL_SERIALIZE( last);
            CASUAL_SERIALIZE( created);
         )

         inline friend bool operator < ( const Queue& lhs, const Queue& rhs)
         {
            if( lhs.type() != rhs.type())
               return lhs.type() < rhs.type();
            return lhs.name < rhs.name;
         }
         inline friend bool operator == ( const Queue& lhs, const std::string name) { return lhs.name == name;}
      };

      namespace forward
      {
         struct Instances
         {
            platform::size::type configured{};
            platform::size::type running{};

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( configured);
               CASUAL_SERIALIZE( running);
            )
         };

         struct Metric
         {
            struct Count
            {
               platform::size::type count{};
               platform::time::point::type last{};

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( count);
                  CASUAL_SERIALIZE( last);
               )
            };

            Count commit;
            Count rollback;

            inline auto transactions() const { return commit.count + rollback.count;}
            inline auto last() const { return std::max( commit.last, rollback.last);}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( commit);
               CASUAL_SERIALIZE( rollback);
            )
         };

         struct Queue
         {
            struct Target
            {
               std::string queue;
               platform::time::unit delay{};

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( queue);
                  CASUAL_SERIALIZE( delay);
               )
            };

            //! the group who owns this queue-forward
            common::strong::process::id group;
            std::string alias;
            std::string source;
            Target target;
            Instances instances;
            Metric metric;
            std::string note;

            inline friend bool operator == ( const Queue& lhs, common::strong::process::id rhs) { return lhs.group == rhs;}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( group);
               CASUAL_SERIALIZE( alias);
               CASUAL_SERIALIZE( source);
               CASUAL_SERIALIZE( target);
               CASUAL_SERIALIZE( instances);
               CASUAL_SERIALIZE( metric);
               CASUAL_SERIALIZE( note);
            )
         };

         struct Service
         {
            struct Target
            {
               std::string service;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( service);
               )
            };

            using Reply = Queue::Target;
            
            //! the group who owns this service-forward
            common::strong::process::id group;
            std::string alias;
            std::string source;
            Target target;
            Instances instances;
            std::optional< Reply> reply;
            Metric metric;
            std::string note;

            inline friend bool operator == ( const Service& lhs, common::strong::process::id rhs) { return lhs.group == rhs;}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( group);
               CASUAL_SERIALIZE( alias);
               CASUAL_SERIALIZE( source);
               CASUAL_SERIALIZE( target);
               CASUAL_SERIALIZE( instances);
               CASUAL_SERIALIZE( reply);
               CASUAL_SERIALIZE( metric);
               CASUAL_SERIALIZE( note);
            )
         };

         struct Group
         {
            std::string alias;
            common::process::Handle process;
            std::string note;

            inline friend bool operator == ( const Group& lhs, common::strong::process::id rhs) { return lhs.process.pid == rhs;}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( alias);
               CASUAL_SERIALIZE( process);
               CASUAL_SERIALIZE( note);
            )
         };

      } // forward

      struct Forward
      {
         std::vector< forward::Group> groups;
         std::vector< forward::Service> services;
         std::vector< forward::Queue> queues;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( groups);
            CASUAL_SERIALIZE( services);
            CASUAL_SERIALIZE( queues);
         )
      };

      struct Message
      {
         enum class State : int
         {
            enqueued = 1,
            committed = 2,
            dequeued = 3,
         };

         common::Uuid id;
         common::strong::queue::id queue;
         common::strong::queue::id origin;
         platform::binary::type trid;
         State state;
         std::string reply;
         platform::size::type redelivered;
         std::string type;

         platform::time::point::type available;
         platform::time::point::type timestamp;

         platform::size::type size;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( id);
            CASUAL_SERIALIZE( queue);
            CASUAL_SERIALIZE( origin);
            CASUAL_SERIALIZE( trid);
            CASUAL_SERIALIZE( state);
            CASUAL_SERIALIZE( reply);
            CASUAL_SERIALIZE( redelivered);
            CASUAL_SERIALIZE( type);
            CASUAL_SERIALIZE( available);
            CASUAL_SERIALIZE( timestamp);
            CASUAL_SERIALIZE( size);
         )
      };

      struct State
      {
         std::vector< Group> groups;
         std::vector< Queue> queues;
         std::vector< Queue> zombies;


         Forward forward;

         struct Remote
         {
            std::vector< remote::Domain> domains;
            std::vector< remote::Queue> queues;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( domains);
               CASUAL_SERIALIZE( queues);
            )

         } remote;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( groups);
            CASUAL_SERIALIZE( queues);
            CASUAL_SERIALIZE( zombies);
            CASUAL_SERIALIZE( forward);
            CASUAL_SERIALIZE( remote);
         )
      };


      struct Affected
      {
         struct Queue
         {
            common::strong::queue::id id;
            std::string name;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( id);
               CASUAL_SERIALIZE( name);
            )
         };
         
         Queue queue;
         platform::size::type count = 0;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( queue);
            CASUAL_SERIALIZE( count);
         )
      };

      namespace scale
      {
         struct Alias
         {
            std::string name;
            platform::size::type instances;

            inline friend bool operator == ( const Alias& lhs, const std::string& rhs) { return lhs.name == rhs;}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( instances);
            )
         };

      } // scale

      } // inline v1

   } // queue::manager::admin::model
} // casual


