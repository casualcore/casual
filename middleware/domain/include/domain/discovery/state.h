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
#include "common/signal/timer.h"

#include <iosfwd>

namespace casual
{
   namespace domain::discovery
   {
      namespace state
      {
         namespace metric::message
         {
            namespace detail
            {
               namespace count
               {
                  enum class Tag : short
                  {
                     received,
                     sent
                  };

                  template< Tag tag, common::message::Type type>
                  struct basic_count
                  {
                     static void increment() noexcept { ++m_count;}
                     static auto value() noexcept { return m_count;}
                  private:
                     static platform::size::type m_count;
                  };

                  template< Tag tag, common::message::Type type>
                  platform::size::type basic_count< tag, type>::m_count = {};


                  template< common::message::Type type>
                  using Receive = basic_count< count::Tag::received, type>;

                  template< common::message::Type type>
                  using Send = basic_count< count::Tag::sent, type>;

               } // count

               template< count::Tag tag, typename C, typename Message>
               constexpr auto create( C creator, const Message&)
               {
                  constexpr auto type = common::message::type< std::remove_cvref_t< Message>>();
                  return creator( 
                     type, 
                     detail::count::basic_count< tag, type>::value());
               }

               template< count::Tag tag, typename C, typename... Messages>
               constexpr auto counts( C creator, Messages... messages) noexcept
               {
                  using value_type = decltype( creator( common::message::Type{}, platform::size::type{}));

                  std::vector< value_type> result;
                  result.reserve( sizeof...( Messages));

                  ( ... , result.push_back( detail::create< tag>( creator, messages)) );

                  return result;
               }

            } // detail

            namespace count
            {
               template< typename Message>
               constexpr void receive( Message&&)
               {
                  detail::count::Receive< common::message::type< std::remove_cvref_t< Message>>()>::increment();
               }

               template< typename C, typename... Messages>
               constexpr auto received( C creator, Messages... messages) noexcept
               {
                  return detail::counts< detail::count::Tag::received>( std::move( creator), std::forward< Messages>( messages)...);
               }

               template< typename Message>
               constexpr void send( Message&&)
               {
                  detail::count::Send< common::message::type< std::remove_cvref_t< Message>>()>::increment();
               }

               template< typename C, typename... Messages>
               constexpr auto sent( C creator, Messages... messages) noexcept
               {
                  return detail::counts< detail::count::Tag::sent>( std::move( creator), std::forward< Messages>( messages)...);
               }
               
            } // count 
            
         } // metric::message


         namespace runlevel
         {
            enum struct State : short
            {
               running,
               shutdown,
            };
            std::string_view description( State value) noexcept;
            
         } // runlevel

         using Runlevel = common::state::Machine< runlevel::State>;

         namespace provider
         {
            using Ability = message::discovery::api::provider::registration::Ability;
            using Abilities = message::discovery::api::provider::registration::Abilities;

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
            namespace topology
            {
               struct Direct
               {
                  std::vector< common::domain::Identity> domains;
                  message::discovery::request::Content configured;

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( domains);
                     CASUAL_SERIALIZE( configured);
                  )
               };

               namespace extract
               {
                  struct Result
                  {
                     message::discovery::topology::direct::Explore direct;
                     std::vector< common::domain::Identity> implicit_domains;

                     CASUAL_LOG_SERIALIZE(
                        CASUAL_SERIALIZE( direct);
                        CASUAL_SERIALIZE( implicit_domains);
                     )
                  };
               } // extract

            } // topology

            struct Topology
            {
               //! will set timer if it's not set. 
               void add( message::discovery::topology::direct::Update&& message);
               void add( message::discovery::topology::implicit::Update&& message);

               std::optional< topology::extract::Result> extract() noexcept;

               //! @returns true if something is aggregated, false otherwise
               inline bool empty() const noexcept { return m_direct.domains.empty() && m_implicit_domains.empty();}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE_NAME( m_direct, "direct");
                  CASUAL_SERIALIZE_NAME( m_implicit_domains, "implicit");
               )
               
            private:
               topology::Direct m_direct;
               std::vector< common::domain::Identity> m_implicit_domains;
            };

            namespace request
            {
               namespace reply
               {
                  enum struct Type : short
                  {
                     api,
                     discovery,
                  };
                  constexpr std::string_view description( Type value) noexcept
                  {
                     switch( value)
                     {
                        case Type::api: return "api";
                        case Type::discovery: return "discovery";
                     }
                     return "<unknown>";
                  }
               } // reply

               struct Reply
               {
                  reply::Type type;
                  common::strong::correlation::id correlation;
                  common::strong::ipc::id ipc;

                  inline friend bool operator == ( const Reply& lhs, reply::Type rhs) noexcept { return lhs.type == rhs;}

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( type);
                     CASUAL_SERIALIZE( correlation);
                     CASUAL_SERIALIZE( ipc);
                  )
               };

               namespace extract
               {
                  struct Result
                  {
                     message::discovery::request::Content content;
                     std::vector< request::Reply> replies;

                     CASUAL_LOG_SERIALIZE(
                        CASUAL_SERIALIZE( content);
                        CASUAL_SERIALIZE( replies);
                     )
                  };
                  
               } // extract
               
            } // request

            struct Request
            {
               void add( message::discovery::Request message);
               void add( message::discovery::api::Request message);

               std::optional< request::extract::Result> extract() noexcept;

               inline auto pending() const noexcept { return m_result.replies.size();}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE_NAME( m_result, "result");
               )
            
            private:
               request::extract::Result m_result;
            };

            namespace extract
            {
               struct Result
               {
                  std::optional< topology::extract::Result> topology;
                  std::optional<request::extract::Result> request;

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( topology);
                     CASUAL_SERIALIZE( request);
                  )
               };
               
            } // extract


            struct Heuristic
            {
               std::optional< common::signal::timer::Deadline> deadline;

               static platform::size::type in_flight() noexcept;

               static const platform::size::type in_flight_window;
               static const platform::time::unit duration;



               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE_NAME( in_flight(), "in_flight");
                  CASUAL_SERIALIZE( in_flight_window);
                  CASUAL_SERIALIZE( duration);
               )
            };


         } // accumulate

         struct Accumulate
         {
            Accumulate();

            void add( message::discovery::Request message);
            void add( message::discovery::api::Request message);
            void add( message::discovery::topology::direct::Update message);
            void add( message::discovery::topology::implicit::Update message);

            //! this should only be called when a timeout occur
            accumulate::extract::Result extract();

            //! @returns true if the accumulate should be bypassed, hence no accumulation based on load.
            bool bypass() const noexcept;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( m_heuristic);
               CASUAL_SERIALIZE( m_topology);
               CASUAL_SERIALIZE( m_request);
            )

         private:
            accumulate::Heuristic m_heuristic;
            accumulate::Topology m_topology;
            accumulate::Request m_request;
         };

         namespace in::flight
         {
            namespace content
            {
               namespace cache
               {
                  struct Mapping
                  {
                     //! adds the resources and associate with the correlation.
                     void add( const common::strong::correlation::id& correlation, const std::vector< std::string>& resources);

                     //! @returns all resources associated with the correlation, and make sure the state is cleaned
                     std::vector< std::string> extract( const common::strong::correlation::id& correlation);

                     //! adds all cached resources to `resources`.
                     void complement( std::vector< std::string>& resources);

                     CASUAL_LOG_SERIALIZE(
                        CASUAL_SERIALIZE( m_correlation_to_resource);
                        CASUAL_SERIALIZE( m_resource_count);
                     )

                  private:
                     std::unordered_multimap< common::strong::correlation::id, std::string> m_correlation_to_resource;
                     std::unordered_map< std::string, platform::size::type> m_resource_count;
                  };

               } // cache

               struct Cache
               {
                  using request_content = message::discovery::request::Content;
                  using reply_content = message::discovery::reply::Content;

                  //! adds the content and associate with the correlation.
                  void add( const common::strong::correlation::id& correlation, const request_content& content);

                  //! adds partial "local" known 'content', that will be used in `filter_reply`
                  void add_known( const common::strong::correlation::id& correlation, reply_content&& content);

                  template< typename M>
                  void add( const M& message) { add( message.correlation, message.content);}

                  //! adds all cached resources to services and queues
                  request_content complement( request_content&& content);

                  //! @returns the "cached" requested resources associated with the correlation intersected with the 
                  //! supplied content. Also adds partial known "local" content, if any. 
                  //! @attention Cleans the associated state -> this function is not idempotent.
                  reply_content filter_reply( const common::strong::correlation::id& correlation, const reply_content& content);

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( m_services);
                     CASUAL_SERIALIZE( m_queues);
                  )

               private:
                  cache::Mapping m_services;
                  cache::Mapping m_queues;
                  std::unordered_map< common::strong::correlation::id, reply_content> m_known_content;
               };
               
            } // content
            
         } // in::flight


      } // state

      struct State
      {
         State();
         
         state::Runlevel runlevel;
         common::communication::select::Directive directive;

         common::communication::ipc::send::Coordinator multiplex{ directive};
         
         struct  
         {
            common::message::coordinate::fan::Out< message::discovery::Reply, common::strong::process::id> discovery;
            common::message::coordinate::fan::Out< message::discovery::lookup::Reply, common::strong::process::id> lookup;
            common::message::coordinate::fan::Out< message::discovery::fetch::known::Reply, common::strong::process::id> known;

            inline void failed( common::strong::process::id pid) { discovery.failed( pid); lookup.failed( pid); known.failed( pid);}
            inline bool empty() const noexcept { return discovery.empty() && lookup.empty() && known.empty();}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( discovery);
               CASUAL_SERIALIZE( lookup);
               CASUAL_SERIALIZE( known);
            )

         } coordinate;

         struct
         {
            std::unordered_map< std::string, std::vector< std::string>> to_routes;
            std::unordered_map< std::string, std::string> to_origin;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( to_routes);
               CASUAL_SERIALIZE( to_origin);
            )
         } service_name;

         state::in::flight::content::Cache in_flight_cache;

         state::Accumulate accumulate;
         
         state::Providers providers;

         bool done() const noexcept;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( coordinate);
            CASUAL_SERIALIZE( multiplex);
            CASUAL_SERIALIZE( service_name);
            CASUAL_SERIALIZE( in_flight_cache);
            CASUAL_SERIALIZE( accumulate);
            CASUAL_SERIALIZE( providers);
         )

      };
   } // domain::discovery:
} // casual