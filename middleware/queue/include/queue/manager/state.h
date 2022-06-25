//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "queue/common/ipc/message.h"

#include "common/communication/ipc.h"
#include "common/domain.h"
#include "common/state/machine.h"


#include "configuration/model.h"

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
         struct Entity : common::Compare< Entity< C>>
         {
            explicit Entity( C configuration) : configuration{ std::move( configuration)}
            {}

            common::state::Machine< entity::Lifetime> state;
            common::process::Handle process;
            C configuration;

            inline friend bool operator == ( const Entity& lhs, common::strong::process::id pid) { return lhs.process.pid == pid;}

            auto tie() const { return std::tie( configuration);}

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

         namespace entity
         {
            inline auto path( const Group&) { return common::process::path().parent_path() / "casual-queue-group";}
            inline auto path( const forward::Group&) { return common::process::path().parent_path() / "casual-queue-forward-group";}
         } // entity


         //! Represent a remote gateway that exports 0..* queues
         struct Remote
         {
            common::process::Handle process;
            platform::size::type order{};

            inline friend bool operator == ( const Remote& lhs, const common::process::Handle& rhs) { return lhs.process == rhs;}
            inline friend bool operator == ( const Remote& lhs, common::strong::process::id pid) { return lhs.process == pid;}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( process);
               CASUAL_SERIALIZE( order);
            )
         };


         struct Queue
         {
            Queue() = default;
            inline Queue( common::process::Handle process, common::strong::queue::id queue, platform::size::type order = 0)
               : process{ std::move( process)}, queue{ queue}, order{ order} {}

            common::process::Handle process;
            common::strong::queue::id queue;
            platform::size::type order{};

            inline auto remote() const { return order > 0;}
            inline auto local() const { return order == 0;}

            inline friend bool operator < ( const Queue& lhs, const Queue& rhs) { return lhs.order < rhs.order;};
            inline friend bool operator == ( const Queue& lhs, common::strong::process::id rhs) { return lhs.process == rhs;}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( process);
               CASUAL_SERIALIZE( queue);
               CASUAL_SERIALIZE( order);
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

         std::unordered_map< std::string, std::vector< state::Queue>> queues;

         struct
         {
            std::deque< ipc::message::lookup::Request> lookups;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( lookups);
            )
         } pending;
         
         std::vector< state::Group> groups;
         
         struct
         {
            std::vector< state::forward::Group> groups;
            
            CASUAL_LOG_SERIALIZE( CASUAL_SERIALIZE( groups);)
         } forward;


         std::vector< state::Remote> remotes;
         
         //! @returns 0..1 queue (providers) that provides the queue
         //! @{
         const state::Queue* queue( const std::string& name) const noexcept;
         const state::Queue* queue( common::strong::queue::id id) const noexcept;

         const state::Queue* local_queue( const std::string& name) const noexcept;
         //! @}

         void update( queue::ipc::message::group::configuration::update::Reply group);
         void update( queue::ipc::message::Advertise& message);

         std::vector< common::strong::process::id> processes() const noexcept;

         //! Removes all queues associated with the process
         //!
         //! @param pid process id
         void remove_queues( common::strong::process::id pid);

         //! Removes the process (group/gateway) and all queues associated with the process
         //!
         //! @param pid process id
         void remove( common::strong::process::id pid);

         

         //! return true if no forwards and queues are running
         bool done() const;

         //! return true if all forwards and queues are at least in running mode.
         bool ready() const;

         std::string note;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( queues);
            CASUAL_SERIALIZE( pending);
            CASUAL_SERIALIZE( groups);
            CASUAL_SERIALIZE( forward);
            CASUAL_SERIALIZE( remotes);
            CASUAL_SERIALIZE( note);
         )

      };
   } // queue::manager
} // casual


