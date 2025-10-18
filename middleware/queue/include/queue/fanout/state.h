//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "queue/common/ipc/message.h"

#include "configuration/model.h"

#include "common/serialize/macro.h"
#include "common/state/machine.h"
#include "common/communication/select.h"
#include "common/communication/ipc/send.h"

#include "casual/container.h"

#include <variant>

namespace casual
{
   namespace queue::fanout
   {
      namespace state
      {
      
         enum struct Runlevel : short
         {
            running,
            shutdown,
            error,
         };
         std::string_view description( Runlevel value);

         struct Metric
         {
            struct Count
            {
               platform::size::type count = 0;
               common::chronology::time_point last{};

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( count);
                  CASUAL_SERIALIZE( last);
               )
            };

            Count commit;
            Count rollback;

            inline auto transactions() const { return commit.count + rollback.count;}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( commit);
               CASUAL_SERIALIZE( rollback);
            )
         };

         struct Profile
         {
            casual::configuration::model::queue::fanout::Queue configuration;
            Metric metric;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( configuration);
               CASUAL_SERIALIZE( metric);
            )
         };

         namespace profile
         {
            using Lookup = casual::container::Index< Profile>;

            using id = typename Lookup::index_type;
            
         } // profile


         namespace machine
         {
            struct base_correlation
            {
               common::strong::correlation::id correlation;
               common::transaction::ID trid;

               inline friend bool operator == ( const base_correlation& lhs, const common::strong::correlation::id& rhs) { return lhs.correlation == rhs;}  

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( correlation);
                  CASUAL_SERIALIZE( trid);
               )
            };

            struct source_lookup : base_correlation
            {
               CASUAL_LOG_SERIALIZE(
                  base_correlation::serialize( archive);
               )
            };

            struct source_lookup_discard 
            {
               common::strong::correlation::id correlation;

               inline friend bool operator == ( const source_lookup_discard& lhs, const common::strong::correlation::id& rhs) { return lhs.correlation == rhs;}  

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( correlation);
               )
            };


            struct source_dequeue : base_correlation
            {
               struct
               {
                  std::string name;
                  common::strong::queue::id id;
                  common::process::Handle group;

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( name);
                     CASUAL_SERIALIZE( id);
                     CASUAL_SERIALIZE( group);
                  )

               } queue;

               CASUAL_LOG_SERIALIZE(
                  base_correlation::serialize( archive);
                  CASUAL_SERIALIZE( queue);
               )
            };

            struct targets_lookup
            {
               common::transaction::ID trid;
               std::vector< common::strong::correlation::id> correlations;
               std::vector< ipc::message::lookup::Reply> replies;

               ipc::message::group::dequeue::Reply message;

               inline friend bool operator == ( const targets_lookup& lhs, const common::strong::correlation::id& rhs) 
               { 
                  return std::ranges::contains( lhs.correlations, rhs); 
               }

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( correlations);
                  CASUAL_SERIALIZE( replies);
                  CASUAL_SERIALIZE( message);
               )
            };

            struct targets_lookup_discard
            {
               common::transaction::ID trid;
               std::vector< common::strong::correlation::id> correlations;

               inline friend bool operator == ( const targets_lookup_discard& lhs, const common::strong::correlation::id& rhs) 
               { 
                  return std::ranges::contains( lhs.correlations, rhs); 
               }

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( trid);
                  CASUAL_SERIALIZE( correlations);
               )
            };

            struct targets_enqueue 
            {
               common::transaction::ID trid;
               std::vector< common::strong::correlation::id> correlations;
               std::vector< ipc::message::group::enqueue::Reply> replies;

               inline friend bool operator == ( const targets_enqueue& lhs, const common::strong::correlation::id& rhs)
               {
                  return std::ranges::contains( lhs.correlations, rhs);
               }

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( correlations);
                  CASUAL_SERIALIZE( replies);
               )
            };

            struct commit : base_correlation
            {
               CASUAL_LOG_SERIALIZE(
                  base_correlation::serialize( archive);
               )
            };
            
            struct rollback : base_correlation
            {
               CASUAL_LOG_SERIALIZE(
                  base_correlation::serialize( archive);
               )
            };
            
         } // machine

         using state_machine_type = std::variant< 
            machine::source_lookup,
            machine::source_lookup_discard,
            machine::source_dequeue,
            machine::targets_lookup,
            machine::targets_lookup_discard,
            machine::targets_enqueue,
            machine::commit,
            machine::rollback>;

         namespace instance
         {
            enum struct Life : short
            {
               restart,
               stop,
            };
            std::string_view description( Life value);

         } // instance

         struct Instance
         {
            profile::id profile;
            state_machine_type state_machine;
            instance::Life life = instance::Life::restart;

            inline friend bool operator == ( const Instance& lhs, const common::strong::correlation::id& rhs) 
            { 
               return std::visit( [&]( auto&& machine){ return machine == rhs;}, lhs.state_machine);
            }

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( profile);
               CASUAL_SERIALIZE( state_machine);
               CASUAL_SERIALIZE( life);
            ) 
         };

         struct Configuration
         {
            //! only to be able to recreate configuration model
            //! @{
            std::string alias;
            std::string note;
            std::vector< std::string> memberships;
            //! @}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( alias);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( memberships);
            )
         };

      } // state

      struct State
      {
         common::state::Machine< state::Runlevel> runlevel;

         common::communication::select::Directive directive;
         common::communication::ipc::send::Coordinator multiplex{ directive};

         std::vector< state::Instance> instances;

         state::profile::Lookup profiles;

         state::Configuration configuration;

         //! @returns transform state to the current configuration model
         casual::configuration::model::queue::fanout::Group configuration_model() const;

         bool done() const;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( directive);
            CASUAL_SERIALIZE( multiplex);
            CASUAL_SERIALIZE( instances);
            CASUAL_SERIALIZE( profiles);
            CASUAL_SERIALIZE( configuration);
         )
      };
      
   } // queue::fanout

} // casual
