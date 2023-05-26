//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/discovery/state.h"

#include "common/environment.h"

namespace casual
{
   using namespace common;
   namespace domain::discovery
   {
      namespace state
      {
         namespace runlevel
         {
            std::string_view description( State value) noexcept
            {
               switch( value)
               {
                  case State::running: return "running";
                  case State::shutdown: return "shutdown";
               }
               return "<unknown>";
            }
         } // runlevel
            
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
            namespace local
            {
               namespace
               {
                  void set_timer( std::optional< common::signal::timer::Deadline>& timer)
                  {
                     if( timer)
                        return;

                     static const auto duration = []() -> platform::time::unit
                     {
                        // check if we're in unittest context or not.
                        if( common::environment::variable::exists( common::environment::variable::name::unittest::context))
                           return std::chrono::milliseconds{ 1};
                        return platform::batch::discovery::accumulate::timeout;
                     }();

                     timer.emplace( duration);
                  }
               } // <unnamed>
            } // local

            void Topology::add( message::discovery::topology::direct::Update&& message)
            {
               Trace trace{ "domain::discovery::state::accumulate::Topology::add"};

               algorithm::append_unique_value( std::move( message.origin), m_direct.domains);
               m_direct.configured += std::move( message.configured);

               local::set_timer( m_timer);
            }

            void Topology::add( message::discovery::topology::implicit::Update&& message)
            {
               Trace trace{ "domain::discovery::state::accumulate::Topology::add"};

               algorithm::append_unique( message.domains, m_implicit.domains);

               // Something is updated at message.origin. We need to include 
               // the origin (connection), in the set we're going to discover later.
               algorithm::append_unique_value( std::move( message.origin), m_direct.domains);

               local::set_timer( m_timer);
            }

            Topology::extract_result Topology::extract() noexcept
            {
               Trace trace{ "domain::discovery::state::accumulate::Topology::extract"};

               // remove deadline regardless. 
               m_timer = std::nullopt;

               extract_result result;
               {
                  auto& [ direct, implicit] = result;

                  direct.domains = std::exchange( m_direct.domains, {});
                  direct.content = std::exchange( m_direct.configured, {});

                  implicit.domains = std::exchange( m_implicit.domains, {});
                  // add our self.
                  algorithm::append_unique_value( common::domain::identity(), implicit.domains);
               }

               log::line( verbose::log, "result: ", result);
               
               return result;
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
         if( runlevel > decltype( runlevel())::running)
            return coordinate.empty();

         return false;
      }

      void State::Prospects::insert( const Content& content, common::strong::correlation::id correlation)
      {
         for( auto service : content.services)
         {
            services[ service].emplace_back( correlation);
         }
         for( auto queue : content.queues)
         {
            queues[ queue].emplace_back( correlation);
         }
      }

      void State::Prospects::remove( common::strong::correlation::id correlation)
      {
         algorithm::container::erase_if( services, [ &correlation]( auto& pair) 
         {
            return algorithm::container::erase( pair.second, correlation).empty();
         });

         algorithm::container::erase_if( queues, [ &correlation]( auto& pair) 
         {
            return algorithm::container::erase( pair.second, correlation).empty();
         });
      }

      State::Prospects::Content State::Prospects::content() const
      {
         State::Prospects::Content content;
         algorithm::transform( services, content.services, []( auto& pair) { return pair.first;});
         algorithm::transform( queues, content.queues, []( auto& pair) { return pair.first;});
         algorithm::container::sort::unique( content.services);
         algorithm::container::sort::unique( content.queues);

         return content;
      }
   } // domain::discovery
} // casual