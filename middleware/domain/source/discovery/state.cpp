//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/discovery/state.h"

#include "common/environment.h"
#include "common/chronology.h"

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
            if( algorithm::find( m_providers, message.process.ipc))
               return;

            m_providers.emplace_back( message.abilities, message.process);

         }

         Providers::const_range_type Providers::filter( provider::Ability abilities) noexcept
         {
            return algorithm::filter( m_providers, [abilities]( auto& provider)
            {
               return predicate::boolean( provider.abilities & abilities);
            });
         }

         void Providers::remove( common::strong::process::id pid)
         {
            algorithm::container::erase( m_providers, pid);
         }

         void Providers::remove( const common::strong::ipc::id& ipc)
         {
            algorithm::container::erase( m_providers, ipc);
         }

         namespace pending
         {
            common::strong::correlation::id Content::add( message::discovery::request::Content content)
            {
               auto correlation = strong::correlation::id::generate();
               m_requests.push_back( pending::content::Request{ correlation, std::move( content)});
               return correlation;
            }

            void Content::remove( const common::strong::correlation::id& correlation)
            {
               algorithm::container::erase( m_requests, correlation);
            }

            message::discovery::request::Content Content::operator() () const
            {
               return algorithm::accumulate( m_requests, message::discovery::request::Content{}, []( auto result, auto& request)
               {
                  return result + request.content;
               });
            }
         } // pending

         namespace accumulate
         {            
            const platform::size::type Heuristic::in_flight_window = []()
            {
               return environment::variable::get< platform::size::type>( environment::variable::name::internal::discovery::accumulate::requests)
                  .value_or( platform::batch::discovery::accumulate::requests);
            }();


            const platform::time::unit Heuristic::duration = []() -> platform::time::unit
            {
               return environment::variable::get( environment::variable::name::internal::discovery::accumulate::timeout)
                  .transform( []( auto value){ return common::chronology::from::string( value);})
                  .value_or( platform::batch::discovery::accumulate::timeout);
            }();

            platform::size::type Heuristic::pending_requests() noexcept
            {
               auto request = common::message::counter::entry( message::discovery::Request::type());
               auto reply = common::message::counter::entry( message::discovery::Reply::type());

               return request.received - reply.sent;
            }

            bool Heuristic::accumulate() const noexcept
            {
               return pending_requests() > in_flight_window;
            }


            namespace local
            {
               namespace
               {
                  auto create_timer( const state::accumulate::Heuristic& heuristic)
                  {
                     auto pending = heuristic.pending_requests();

                     // this may happen since we always accumulate topology requests
                     if( pending <= heuristic.in_flight_window)
                        return common::signal::timer::Deadline{ heuristic.duration};

                     auto load_level = pending / heuristic.in_flight_window;
                     log::line( verbose::log, "load_level: ", load_level);
                     return common::signal::timer::Deadline{ heuristic.duration * load_level};
                  }


               } // <unnamed>
            } // local

         } // accumulate

         void Accumulate::add( message::discovery::Request message)
         {
            m_requests.discovery.push_back( std::move( message));

            if( ! m_deadline)
               m_deadline = accumulate::local::create_timer( m_heuristic);
         }

         void Accumulate::add( message::discovery::api::Request message)
         {
            m_requests.api.push_back( std::move( message));

            if( ! m_deadline)
               m_deadline = accumulate::local::create_timer( m_heuristic);

         }

         void Accumulate::add( message::discovery::topology::direct::Update message)
         {
            m_requests.direct.push_back( std::move( message));

            if( ! m_deadline)
               m_deadline = accumulate::local::create_timer( m_heuristic);

         }

         void Accumulate::add( message::discovery::topology::implicit::Update message)
         {
            m_requests.implicit.push_back( std::move( message));

            if( ! m_deadline)
               m_deadline = accumulate::local::create_timer( m_heuristic);
         }

         void Accumulate::add( message::discovery::reply::Content lookup)
         {
            m_requests.lookup += std::move( lookup);
         }


         accumulate::Requests Accumulate::extract()
         {
            Trace trace{ "domain::discovery::state::accumulate::Accumulate::extract"};
            
            m_deadline = std::nullopt;
            return std::exchange( m_requests, {});
         }

         Accumulate::operator bool() const noexcept
         {
            if( m_deadline)
               return true;

            return m_heuristic.accumulate();
         }
      } // state

      State::State()
      {
         // make sure we add the inbound ipc for read.
         directive.read.add( communication::ipc::inbound::device().descriptor());
      }

      void State::failed( strong::process::id pid)
      {
         providers.remove( pid);
         coordinate.failed( pid);
      }

      void State::failed( const strong::ipc::id& ipc)
      {
         providers.remove( ipc);
         coordinate.failed( ipc);
      }
      
      bool State::done() const noexcept
      {
         if( runlevel > decltype( runlevel())::running)
            return coordinate.empty();

         return false;
      }

   } // domain::discovery
} // casual