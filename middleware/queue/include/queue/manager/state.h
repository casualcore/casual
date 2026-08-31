//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "queue/common/ipc/message.h"

#include "casual/task.h"

#include "common/communication/select.h"
#include "common/communication/ipc/send.h"
#include "common/domain.h"
#include "common/state/machine.h"

#include "casual/manager/service/context.h"
#include "casual/manager/service/policy.h"

#include "configuration/model.h"
#include "configuration/group.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace casual
{
   namespace queue::manager
   {

      namespace state
      {
         namespace entity
         {
            enum struct Lifetime : short
            {
               absent,
               spawned,
               connected,
               running,
               shutdown,
            };
            std::string_view description( Lifetime value);
            
         } // entity

         template< typename C>
         struct Entity
         {
            explicit Entity( C configuration) : configuration{ std::move( configuration)}
            {}

            common::state::Machine< entity::Lifetime> state;
            common::process::Handle process;
            C configuration;

            inline friend bool operator == ( const Entity& lhs, const std::string& rhs) { return lhs.configuration.alias == rhs;}
            inline friend bool operator == ( const Entity& lhs, common::process::compare_equal_to_handle auto rhs) { return lhs.process.pid == rhs;}
            inline friend bool operator == ( const Entity&, const Entity&) = default;
            inline friend auto operator <=> ( const Entity&, const Entity&) = default;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( state);
               CASUAL_SERIALIZE( process);
               CASUAL_SERIALIZE( configuration);
            )
         };

         using Group = Entity< configuration::model::queue::Group>;
         namespace forward
         {
            using Group = Entity< configuration::model::queue::forward::Group>;
         }

         namespace fanout
         {
            using Group = Entity< configuration::model::queue::fanout::Group>;
         } // fanout

         namespace entity
         {
            inline auto path( const Group&) { return common::process::path().parent_path() / "casual-queue-group";}
            inline auto path( const forward::Group&) { return common::process::path().parent_path() / "casual-queue-forward-group";}
            inline auto path( const fanout::Group&) { return common::process::path().parent_path() / "casual-queue-fanout-group";}
         } // entity


         //! Represent a remote gateway that exports 0..* queues
         struct Remote
         {
            common::process::Handle process;
            std::vector< common::strong::correlation::id> reservations;
            platform::size::type order{};
            std::string alias;
            std::string description;

            void reserve( const common::strong::correlation::id& correlation);
            bool unreserve( const common::strong::correlation::id& correlation);

            inline friend bool operator == ( const Remote& lhs, common::process::compare_equal_to_handle auto rhs) { return lhs.process == rhs;}
            inline friend bool operator == ( const Remote& lhs, const common::strong::correlation::id& rhs) { return std::ranges::contains( lhs.reservations, rhs);}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( process);
               CASUAL_SERIALIZE( reservations);
               CASUAL_SERIALIZE( order);
               CASUAL_SERIALIZE( alias);
               CASUAL_SERIALIZE( description);
            )            
         };

      
         struct Queue
         {
            using Enable = casual::configuration::model::queue::Queue::Enable;

            common::process::Handle process;
            common::strong::queue::id queue;
            platform::size::type order{};
            Enable enable;

            inline auto remote() const { return order > 0;}
            inline auto local() const { return order == 0;}

            inline friend bool operator < ( const Queue& lhs, const Queue& rhs) { return lhs.order < rhs.order;};
            inline friend bool operator == ( const Queue& lhs, common::process::compare_equal_to_handle auto rhs) { return lhs.process == rhs;}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( process);
               CASUAL_SERIALIZE( queue);
               CASUAL_SERIALIZE( order);
               CASUAL_SERIALIZE( enable);
            )
         };


         struct Task
         {
            casual::task::Coordinator coordinator;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( coordinator);
            )
         };

         enum struct Runlevel : short
         {
            running,
            shutdown,
            error,
         };
         std::string_view description( Runlevel value);
         
      } // state

      struct State
      {
         common::state::Machine< state::Runlevel> runlevel;

         common::communication::select::Directive directive;
         common::communication::ipc::send::Coordinator multiplex{ directive};

         std::unordered_map< std::string, std::vector< state::Queue>> queues;

         configuration::group::Coordinator group_coordinator;

         struct
         {
            std::deque< ipc::message::lookup::Request> lookups;
            std::vector< ipc::message::external::disassociate::Request> disassociation;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( lookups);
               CASUAL_SERIALIZE( disassociation);
            )
         } pending;
         
         std::vector< state::Group> groups;
         
         struct
         {
            std::vector< state::forward::Group> groups;
            
            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( groups);
            )
         } forward;

         struct
         {
            std::vector< state::fanout::Group> groups;
            
            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( groups);
            )
         } fanout;


         std::vector< state::Remote> remotes;

         state::Task task;

         casual::manager::service::Context< casual::manager::service::policy::Default> services;


         //! @returns 0..1 queue (providers) that provides the queue
         //! @{
         const state::Queue* queue( const std::string& name, queue::ipc::message::lookup::request::context::Action action) const noexcept;
         const state::Queue* local_queue( const std::string& name, queue::ipc::message::lookup::request::context::Action action) const noexcept;
         //! @}

         void update( queue::ipc::message::group::configuration::update::Reply group);
         void update( queue::ipc::message::Advertise& message);

         //! Removes all queues associated with the process
         //!
         //! @param pid process id
         void remove_queues( common::strong::process::id pid);
         void remove_queues( common::strong::ipc::id ipc);

         //! Removes all associated state with the process.
         void remove( common::strong::process::id pid);

         //! Removes all associated state with the ipc.
         void remove( common::strong::ipc::id ipc);

         //! return true if no forwards and queues are running
         bool done() const;

         std::string note;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( directive);
            CASUAL_SERIALIZE( multiplex);
            CASUAL_SERIALIZE( queues);
            CASUAL_SERIALIZE( pending);
            CASUAL_SERIALIZE( groups);
            CASUAL_SERIALIZE( forward);
            CASUAL_SERIALIZE( fanout);
            CASUAL_SERIALIZE( remotes);
            CASUAL_SERIALIZE( task);
            CASUAL_SERIALIZE( note);
         )
      };

   } // queue::manager
} // casual


