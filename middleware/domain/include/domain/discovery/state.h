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
         std::ostream& operator << ( std::ostream& out, Runlevel value);

         //! represent an instance that can discover
         struct Agent
         {
            enum struct Bound : short
            {
               inbound,
               outbound,
               outbound_rediscover,
            };
            friend std::ostream& operator << ( std::ostream& out, Bound value);

            Bound bound{};
            common::process::Handle process;

            inline friend bool operator == ( const Agent& lhs, Bound rhs) { return lhs.bound == rhs;}
            inline friend bool operator == ( const Agent& lhs, const common::process::Handle& rhs) { return lhs.process == rhs;}

            inline friend bool operator < ( const Agent& lhs, const Agent& rhs) { return lhs.bound < rhs.bound;}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( bound);
               CASUAL_SERIALIZE( process);
            )
         };

         //! manages all 'agents' that can do discovery in some way
         struct Agents
         {
            using const_range_type = common::range::const_type_t< std::vector< state::Agent>>;
            
            void registration( const message::discovery::inbound::Registration& message);
            void registration( const message::discovery::outbound::Registration& message);

            const_range_type inbounds() const;
            const_range_type outbounds() const;
            const_range_type rediscovers() const;

            inline auto& all() const { return m_agents;}

            void remove( common::strong::process::id pid);

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_agents, "agents");
            )

         private:
            std::vector< state::Agent> m_agents;
         };

      } // state
      struct State
      {
         common::state::Machine< state::Runlevel> runlevel;
         
         struct 
         {
            common::message::coordinate::fan::Out< message::discovery::Reply, common::strong::process::id> discovery;
            common::message::coordinate::fan::Out< message::discovery::rediscovery::Reply, common::strong::process::id> rediscovery;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( discovery);
               CASUAL_SERIALIZE( rediscovery);
            )

         } coordinate;
         
         state::Agents agents;


         bool done() const noexcept;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( coordinate);
            CASUAL_SERIALIZE( agents);
         )

      };
   } // domain::discovery:
} // casual