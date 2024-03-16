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
            // if we have an ongoing deadline we keep accumulation until the deadline kicks in.
            if( m_heuristic.deadline)
               return false;

            // bypass if in-flight is within the "window", the lowest/first load level
            return m_heuristic.in_flight() <= m_heuristic.in_flight_window;
         }

         namespace in::flight
         {
            namespace content
            {
               namespace cache
               {
                  void Mapping::add( const common::strong::correlation::id& correlation, const std::vector< std::string>& resources)
                  {
                     for( auto& resource : resources)
                     {
                        m_correlation_to_resource.emplace( correlation, resource);
                        ++m_resource_count[ resource];
                     }
                  }

                  std::vector< std::string> Mapping::extract( const common::strong::correlation::id& correlation)
                  {
                     std::vector< std::string> result;

                     auto make_range = []( auto pair) { return range::make( pair.first, pair.second);};

                     auto resources = make_range( m_correlation_to_resource.equal_range( correlation));

                     result.reserve( resources.size());

                     for( auto& pair : resources)
                     {
                        // clean upp total "cached" services
                        if( auto found = algorithm::find( m_resource_count, pair.second); --found->second == 0)
                           m_resource_count.erase( std::begin( found));

                        result.push_back( std::move( pair.second));
                     };

                     // clean upp the resources associated with the correlation
                     m_correlation_to_resource.erase( std::begin( resources), std::end( resources));

                     // make sure we're sorted, since the "hash-mapping" is not.
                     algorithm::sort( result);

                     return result;
                  }

                  void Mapping::complement( std::vector< std::string>& resources)
                  {
                     // just a premature optimization if we have a lot of services cached...
                     resources.reserve( m_resource_count.size());

                     algorithm::transform( m_resource_count, std::back_inserter( resources), predicate::adapter::first());
                     algorithm::container::sort::unique( resources);
                  }

               } // cache

               void Cache::add( const common::strong::correlation::id& correlation, const request_content& content)
               {
                  m_services.add( correlation, content.services);
                  m_queues.add( correlation, content.queues);
               }

               void Cache::add_known( const common::strong::correlation::id& correlation, reply_content&& content)
               {
                  m_known_content.emplace( correlation, std::move( content));
               }

               Cache::request_content Cache::complement( request_content&& content)
               {
                  m_services.complement( content.services);
                  m_queues.complement( content.queues);
                  return std::move( content);
               }
               

               Cache::reply_content Cache::filter_reply( const common::strong::correlation::id& correlation, const reply_content& content)
               {
                  Trace trace{ "domain::discovery::state::in::flight::content::Cache::filter_reply"};
                  log::line( verbose::log, "correlation: ", correlation);
                  log::line( verbose::log, "content: ", content);

                  // trim the content with the intersection of the replied end the associated (requested).
                  // also `extract` removes the associated state from the state.

                  reply_content result;

                  //! check if we know stuff locally, if so we add this.
                  if( auto found = algorithm::find( m_known_content, correlation))
                     result = algorithm::container::extract( m_known_content, std::begin( found)).second;


                  auto resource_name_less = []( auto& resource, auto& name){ return resource.name < name;};

                  for( auto& service : m_services.extract( correlation))
                  {
                     if( auto found = std::get< 1>( algorithm::sorted::lower_bound( content.services, service, resource_name_less)))
                        if( found->name == service)
                           result.services.push_back( *found);
                  }

                  for( auto& queue : m_queues.extract( correlation))
                  {
                     if( auto found = std::get< 1>( algorithm::sorted::lower_bound( content.queues, queue, resource_name_less)))
                        if( found->name == queue)
                           result.queues.push_back( *found);
                  }

                  // make sure we keep the invariants, the 'resources' needs to be sorted.
                  // TODO: The sorted stuff might be to much hassle, and we don't really know the performance impact...
                  algorithm::sort( result.services);
                  algorithm::sort( result.queues);

                  return result;  
               }

            } // content
            
         } // in::flight
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