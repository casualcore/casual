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
                  constexpr auto type = common::message::type< common::traits::remove_cvref_t< Message>>();
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
                  detail::count::Receive< common::message::type< common::traits::remove_cvref_t< Message>>()>::increment();
               }

               template< typename C, typename... Messages>
               constexpr auto received( C creator, Messages... messages) noexcept
               {
                  return detail::counts< detail::count::Tag::received>( std::move( creator), std::forward< Messages>( messages)...);
               }

               template< typename Message>
               constexpr void send( Message&&)
               {
                  detail::count::Send< common::message::type< common::traits::remove_cvref_t< Message>>()>::increment();
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
            namespace topology::state
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

               struct Implicit
               {
                  std::vector< common::domain::Identity> domains;

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( domains);
                  )
               };
            } // topology::state

            struct Topology
            {
               //! will set timer if it's not set. 
               void add( message::discovery::topology::direct::Update&& message);
               void add( message::discovery::topology::implicit::Update&& message);

               using extract_result = std::tuple< message::discovery::topology::direct::Explore, message::discovery::topology::implicit::Update>;
               extract_result extract() noexcept;

               //! @returns true if something is aggregated, false otherwise
               inline explicit operator bool() const noexcept { return ! m_direct.domains.empty() || ! m_implicit.domains.empty();}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE_NAME( m_direct, "direct");
                  CASUAL_SERIALIZE_NAME( m_implicit, "implicit");
               )
               
            private:

               topology::state::Direct m_direct;
               topology::state::Implicit m_implicit;
               //! keeps track of the lifetime of our timer.
               std::optional< common::signal::timer::Deadline> m_timer;
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
         
         state::Runlevel runlevel;
         common::communication::select::Directive directive;

         common::communication::ipc::send::Coordinator multiplex{ directive};
         
         struct  
         {
            common::message::coordinate::fan::Out< message::discovery::Reply, common::strong::process::id> discovery;
            common::message::coordinate::fan::Out< message::discovery::internal::Reply, common::strong::process::id> internal;
            common::message::coordinate::fan::Out< message::discovery::known::Reply, common::strong::process::id> known;

            inline void failed( common::strong::process::id pid) { discovery.failed( pid); internal.failed( pid); known.failed( pid);}
            inline bool empty() const noexcept { return discovery.empty() && internal.empty() && known.empty();}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( discovery);
               CASUAL_SERIALIZE( internal);
               CASUAL_SERIALIZE( known);
            )

         } coordinate;

         state::Accumulate accumulate;
         
         state::Providers providers;

         bool done() const noexcept;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( coordinate);
            CASUAL_SERIALIZE( multiplex);
            CASUAL_SERIALIZE( accumulate);
            CASUAL_SERIALIZE( providers);
         )

      };
   } // domain::discovery:
} // casual