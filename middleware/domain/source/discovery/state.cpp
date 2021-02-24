//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/discovery/state.h"

namespace casual
{
   using namespace common;
   namespace domain::discovery
   {
      namespace state
      {
         std::ostream& operator << ( std::ostream& out, Runlevel value)
         {
            switch( value)
            {
               case Runlevel::running: return out << "running";
               case Runlevel::shutdown: return out << "shutdown";
            }
            return out << "<unknown>";
         }

         std::ostream& operator << ( std::ostream& out, Agent::Bound value)
         {
            return out << "bla";
         }
            
         void Agents::registration( const message::discovery::inbound::Registration& message)
         {
            // we only add 'new' processes
            if( algorithm::find( m_agents, message.process))
               return;

            m_agents.push_back( Agent{ Agent::Bound::inbound, message.process});
            algorithm::sort( m_agents);
         }

         void Agents::registration( const message::discovery::outbound::Registration& message)
         {
            // we only add 'new' processes
            if( algorithm::find( m_agents, message.process))
               return;

            if( message.directive == decltype( message.directive)::rediscovery)
                m_agents.push_back( Agent{ Agent::Bound::outbound_rediscover, message.process});
            else 
               m_agents.push_back( Agent{ Agent::Bound::outbound, message.process});
            algorithm::sort( m_agents);
         }

         Agents::const_range_type Agents::inbounds() const
         {
            return range::make( 
               std::begin( m_agents),
               std::begin( algorithm::find( m_agents, Agent::Bound::outbound)));
         }

         Agents::const_range_type Agents::outbounds() const
         {
            return range::make( 
               std::begin( algorithm::find( m_agents, Agent::Bound::outbound)),
               std::end( m_agents));
         }

         Agents::const_range_type Agents::rediscovers() const
         {
            return range::make( 
               std::begin( algorithm::find( m_agents, Agent::Bound::outbound_rediscover)),
               std::end( m_agents));
         }


         void Agents::remove( common::strong::process::id pid)
         {
            common::algorithm::trim( m_agents, common::algorithm::remove_if( m_agents, [pid]( auto& point)
            {
               return point.process == pid;
            }));
         }


      } // state
      
      bool State::done() const noexcept
      {
         if( runlevel == decltype( runlevel())::running)
            return false;

         return coordinate.discovery.empty() && coordinate.rediscovery.empty();
      }

      
   } // domain::discovery:
} // casual