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

         namespace agent
         {
            std::ostream& operator << ( std::ostream& out, Bound value)
            {
               switch( value)
               {
                  case Bound::internal: return out << "internal";
                  case Bound::external: return out << "external";
                  case Bound::external_rediscover: return out << "external_rediscover";
               }
               return out << "<unknown>";
            }            
         } // agent

            
         void Agents::registration( const message::discovery::internal::registration::Request& message)
         {
            // we only add 'new' processes
            if( algorithm::find( m_agents, message.process))
               return;

            m_agents.emplace_back( agent::Bound::internal, message.process);
            algorithm::sort( m_agents);
         }

         void Agents::registration( const message::discovery::external::registration::Request& message)
         {
            // we only add 'new' processes
            if( algorithm::find( m_agents, message.process))
               return;

            if( message.directive == decltype( message.directive)::rediscovery)
                m_agents.emplace_back( agent::Bound::external_rediscover, message.process);
            else 
               m_agents.emplace_back( agent::Bound::external, message.process);
            algorithm::sort( m_agents);
         }

         Agents::const_range_type Agents::internal() const
         {
            return range::make( 
               std::begin( m_agents),
               std::begin( algorithm::find( m_agents, agent::Bound::external)));
         }

         Agents::const_range_type Agents::external() const
         {
            return range::make( 
               std::begin( algorithm::find( m_agents, agent::Bound::external)),
               std::end( m_agents));
         }

         Agents::const_range_type Agents::rediscover() const
         {
            return range::make( 
               std::begin( algorithm::find( m_agents, agent::Bound::external_rediscover)),
               std::end( m_agents));
         }


         void Agents::remove( common::strong::process::id pid)
         {
            common::algorithm::trim( m_agents, common::algorithm::remove( m_agents, pid));
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