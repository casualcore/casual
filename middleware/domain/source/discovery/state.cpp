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

            void Topology::add( message::discovery::topology::direct::Update&& message)
            {
               Trace trace{ "domain::discovery::state::accumulate::Topology::add"};

               algorithm::append_unique_value( std::move( message.origin), m_direct.domains);
               m_direct.configured += std::move( message.configured);

            }

            void Topology::add( message::discovery::topology::implicit::Update&& message)
            {
               Trace trace{ "domain::discovery::state::accumulate::Topology::add"};

               algorithm::append_unique( message.domains, m_implicit_domains);

               // Something is updated at message.origin. We need to include 
               // the origin (connection), in the set we're going to discover later.
               algorithm::append_unique_value( std::move( message.origin), m_direct.domains);
            }

            std::optional< topology::extract::Result> Topology::extract() noexcept
            {
               Trace trace{ "domain::discovery::state::accumulate::Topology::extract"};

               // We could have zero accumulated
               if( empty())
                  return std::nullopt;

               topology::extract::Result result;
               {
                  result.direct.domains = std::exchange( m_direct.domains, {});
                  result.direct.content = std::exchange( m_direct.configured, {});

                  result.implicit_domains = std::exchange( m_implicit_domains, {});
               }

               log::line( verbose::log, "result: ", result);
               
               return result;
            }

            void Request::add( message::discovery::Request message)
            {
               Trace trace{ "domain::discovery::state::accumulate::Request::add discovery"};

               if( message.directive != decltype( message.directive)::forward)
                  log::line( log::category::error, code::casual::internal_unexpected_value, " unexpected discovery directive: ", message.directive);

               m_result.content += std::move( message.content);
               m_result.replies.push_back( { request::reply::Type::discovery, message.correlation, message.process.ipc});
            }

            void Request::add( message::discovery::api::Request message)
            {
               Trace trace{ "domain::discovery::state::accumulate::Request::add api"};

               m_result.content += std::move( message.content);
               m_result.replies.push_back( { request::reply::Type::api, message.correlation, message.process.ipc});
            }

            std::optional< request::extract::Result> Request::extract() noexcept
            {
               Trace trace{ "domain::discovery::state::accumulate::Request::extract"};

               // We could have zero accumulated
               if( m_result.replies.size() == 0)
                  return std::nullopt;

               return std::exchange( m_result, {});
            }
            
            const platform::size::type Heuristic::in_flight_window = []()
            {
               constexpr std::string_view environment = "CASUAL_DISCOVERY_ACCUMULATE_REQUESTS";

               if( environment::variable::exists( environment))
                  return string::from< platform::size::type>( environment::variable::get( environment));

               return platform::batch::discovery::accumulate::requests;
            }();

            const platform::time::unit Heuristic::duration = []() -> platform::time::unit
            {
               constexpr std::string_view environment = "CASUAL_DISCOVERY_ACCUMULATE_TIMEOUT";

               if( environment::variable::exists( environment))
                  return common::chronology::from::string( environment::variable::get( environment));

               return platform::batch::discovery::accumulate::timeout;
            }();

            platform::size::type Heuristic::in_flight() noexcept
            {
               using discovery_sent = state::metric::message::detail::count::Send< message::discovery::Request::type()>;
               using discovery_received = state::metric::message::detail::count::Receive< message::discovery::Reply::type()>;

               return discovery_sent::value() - discovery_received::value();
            }


            namespace local
            {
               namespace
               {
                  void potentially_set_timer( state::accumulate::Heuristic& heuristic)
                  {
                     if( heuristic.deadline)
                        return;

                     auto in_flight = heuristic.in_flight();

                     // sanity check. We should never have reach this with low in-flights
                     if( in_flight <= heuristic.in_flight_window)
                     {
                        log::line( log::category::error, code::casual::invalid_semantics, " unexpected inflight: ", heuristic.in_flight());
                        log::line( log::category::verbose::error, "heuristic: ", heuristic );

                        heuristic.deadline.emplace( heuristic.duration);
                        return;
                     }

                     auto load_level = in_flight / heuristic.in_flight_window;

                     log::line( verbose::log, "load_level: ", load_level);

                     heuristic.deadline.emplace( heuristic.duration * load_level);
                  }
               } // <unnamed>
            } // local

         } // accumulate

         Accumulate::Accumulate()
         {}

         void Accumulate::add( message::discovery::Request message)
         {
            m_request.add( std::move( message));
            accumulate::local::potentially_set_timer( m_heuristic);
         }

         void Accumulate::add( message::discovery::api::Request message)
         {
            m_request.add( std::move( message));
            accumulate::local::potentially_set_timer( m_heuristic);
         }

         void Accumulate::add( message::discovery::topology::direct::Update message)
         {
            m_topology.add( std::move( message));
            accumulate::local::potentially_set_timer( m_heuristic);

         }

         void Accumulate::add( message::discovery::topology::implicit::Update message)
         {
            m_topology.add( std::move( message));
            accumulate::local::potentially_set_timer( m_heuristic);
         }


         accumulate::extract::Result Accumulate::extract()
         {
            Trace trace{ "domain::discovery::state::accumulate::Accumulate::extract"};

            // always invalidate the timeout, if any.
            m_heuristic.deadline = std::nullopt;

            return { m_topology.extract(), m_request.extract()};
         }

         bool Accumulate::bypass() const noexcept
         {
            Trace trace{ "domain::discovery::state::Accumulate::bypass"};


            // if we have an ongoing deadline we keep accumulation until the deadline kicks in.
            if( m_heuristic.deadline)
               return false;

            log::line( verbose::log, "bypass-heuristic: ", m_heuristic);

            // bypass if in-flight is within the "window", the lowest/first load level
            return m_heuristic.in_flight() <= m_heuristic.in_flight_window;
         }

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