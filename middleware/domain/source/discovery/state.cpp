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
         std::string_view description( Runlevel value)
         {
            switch( value)
            {
               case Runlevel::running: return "running";
               case Runlevel::shutdown: return "shutdown";
            }
            return "<unknown>";
         }
            
         void Providers::registration( const message::discovery::api::provider::registration::Request& message)
         {
            // we only add 'new' processes
            if( algorithm::find( m_providers, message.process))
               return;

            m_providers.emplace_back( message.abilities, message.process);

         }

         Providers::const_range_type Providers::filter( provider::Abilities abilities) noexcept
         {
            return algorithm::filter( m_providers, [abilities]( auto& provider)
            {
               return predicate::boolean( provider.abilities & abilities);
            });
         }

         void Providers::remove( common::strong::process::id pid)
         {
            common::algorithm::container::trim( m_providers, common::algorithm::remove( m_providers, pid));
         }

         namespace accumulate
         {
            void Topology::add( std::vector< common::domain::Identity> domains)
            {
               // add this domain, since we by definition has seen this topology update (or others before)
               if( auto position = std::get< 1>( algorithm::sorted::lower_bound( m_domains, common::domain::identity())))
               {
                  if( *position != common::domain::identity())
                     m_domains.insert( std::begin( position), common::domain::identity());
               }
               else
                  m_domains.push_back( common::domain::identity());

               algorithm::sorted::append_unique( algorithm::sort( domains), m_domains);
            }

            std::vector< common::domain::Identity> Topology::extract() noexcept
            {
               m_count = 0;
               return std::exchange( m_domains, {});
            }

         } // accumulate
      } // state

      State::State()
      {
         // make sure we add the inbound ipc for read.
         directive.read.add( communication::ipc::inbound::device().descriptor());
      }
      
      bool State::done() const noexcept
      {
         if( runlevel == decltype( runlevel())::running)
            return false;

         return coordinate.empty();
      }

      
   } // domain::discovery:
} // casual