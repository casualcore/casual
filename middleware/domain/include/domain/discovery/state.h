//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "domain/message/discovery.h"

#include "common/state/machine.h"
#include "common/serialize/macro.h"
#include "common/process.h"
#include "common/message/coordinate.h"
#include "common/communication/select.h"
#include "common/communication/ipc/send.h"

#include <iosfwd>

namespace casual
{
   namespace domain::discovery
   {
      namespace state
      {
         enum struct Runlevel : short
         {
            running,
            shutdown,
         };
         std::string_view description( Runlevel value);

         namespace provider
         {
            using Ability = message::discovery::api::provider::registration::Ability;
            using Abilities = common::Flags< Ability>;

         } // provider

         //! represent an entity that can provide stuff
         struct Provider
         {
            Provider( provider::Abilities abilities, const common::process::Handle& process)
               : abilities{ abilities}, process{ process} {}

            provider::Abilities abilities{};
            common::process::Handle process;

            inline friend bool operator == ( const Provider& lhs, const common::process::Handle& rhs) { return lhs.process == rhs;}
            inline friend bool operator == ( const Provider& lhs, common::strong::process::id rhs) { return lhs.process.pid == rhs;}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( abilities);
               CASUAL_SERIALIZE( process);
            )
         };

         //! manages all 'agents' that can do discovery in some way
         struct Providers
         {
            using const_range_type = common::range::const_type_t< std::vector< state::Provider>>;
            
            void registration( const message::discovery::api::provider::registration::Request& message);

            const_range_type filter( provider::Abilities abilities) noexcept;

            inline auto& all() const noexcept { return m_providers;}

            void remove( common::strong::process::id pid);

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_providers, "providers");
            )

         private:
            std::vector< state::Provider> m_providers;
         };

         namespace accumulate
         {
            struct Request
            {
               struct Destination
               {
                  common::strong::correlation::id correlation;
                  common::strong::ipc::id ipc;
               };

               message::discovery::Request content;
               std::vector< Destination> destinations;
            };

            struct Topology
            {
               void add( std::vector< common::domain::Identity> domains);

               inline explicit operator bool() const noexcept { return ! m_domains.empty();}

               std::vector< common::domain::Identity> extract() noexcept;

               inline bool limit() const noexcept { return m_count > platform::batch::discovery::topology::updates;}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE_NAME( m_domains, "domains");
               )

            private:
               platform::size::type m_count{};
               std::vector< common::domain::Identity> m_domains;
            };
         } // accumulate

         struct Accumulate
         {
            accumulate::Topology topology;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( topology);
            )
         };

      } // state


      struct State
      {
         State();
         
         common::state::Machine< state::Runlevel> runlevel;
         common::communication::select::Directive directive;
         
         struct 
         {
            common::message::coordinate::fan::Out< message::discovery::Reply, common::strong::process::id> discovery;
            common::message::coordinate::fan::Out< message::discovery::needs::Reply, common::strong::process::id> needs;

            inline void failed( common::strong::process::id pid) { discovery.failed( pid); needs.failed( pid);}
            inline bool empty() const noexcept { return discovery.empty() && needs.empty();}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( discovery);
               CASUAL_SERIALIZE( needs);
            )

         } coordinate;

         state::Accumulate accumulate;
         
         state::Providers providers;

         common::communication::ipc::send::Coordinator multiplex{ directive};


         bool done() const noexcept;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( coordinate);
            CASUAL_SERIALIZE( accumulate);
            CASUAL_SERIALIZE( providers);
            CASUAL_SERIALIZE( multiplex);
         )

      };
   } // domain::discovery:
} // casual