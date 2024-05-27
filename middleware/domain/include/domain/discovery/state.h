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
         } // provider

         //! represent an entity that can provide stuff
         struct Provider
         {
            Provider( provider::Ability abilities, const common::process::Handle& process)
               : abilities{ abilities}, process{ process} {}

            provider::Ability abilities{};
            common::process::Handle process;

            friend bool operator == ( const Provider& lhs, common::process::compare_equal_to_handle auto rhs) { return lhs.process == rhs;}


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

            const_range_type filter( provider::Ability abilities) noexcept;

            inline auto& all() const noexcept { return m_providers;}

            void remove( const common::strong::ipc::id& ipc);
            void remove( common::strong::process::id pid);

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_providers, "providers");
            )

         private:
            std::vector< state::Provider> m_providers;
         };

         namespace accumulate
         {
            struct Heuristic
            {
               
               //! @return the number of pending discovery requests
               static platform::size::type pending_requests() noexcept;

               //! @return true if the heuristics say we need to accumulate
               bool accumulate() const noexcept;

               static const platform::size::type in_flight_window;
               static const platform::time::unit duration;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE_NAME( pending_requests(), "pending_requests");
                  CASUAL_SERIALIZE( in_flight_window);
                  CASUAL_SERIALIZE( duration);
               )
            };

            struct Requests
            {
               std::vector< message::discovery::Request> discovery;
               std::vector< message::discovery::api::Request> api;
               std::vector< message::discovery::topology::implicit::Update> implicit;
               std::vector< message::discovery::topology::direct::Update> direct;
               message::discovery::reply::Content lookup;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( discovery);
                  CASUAL_SERIALIZE( api);
                  CASUAL_SERIALIZE( implicit);
                  CASUAL_SERIALIZE( direct);
                  CASUAL_SERIALIZE( lookup);
               )
            };


         } // accumulate

         namespace pending
         {
            namespace content
            {
               struct Request
               {
                  common::strong::correlation::id correlation;
                  message::discovery::request::Content content;

                  inline friend bool operator == ( const Request& lhs, const common::strong::correlation::id& rhs) { return lhs.correlation == rhs;}

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( correlation);
                     CASUAL_SERIALIZE( content);
                  )

               };
               
            } // content

            //! Holds content for pending discovery requests. This
            //! is used to complement requests for new connections with the union of these pending ones,
            //! to eliminate the possibility that a service/queue is not yet known, but a discovery is in
            //! flight. If the not-yet known service is not in the request for the new connection, we might
            //! never know that the new connection has the given service/queue (if we not use this pending content)
            struct Content
            {
               //! adds `content` and @returns a correlation to be used in remove
               //! @attention the `content` should be normalized (resolved routes)
               common::strong::correlation::id add( message::discovery::request::Content content);
               void remove( const common::strong::correlation::id& correlation);

               //! @returns the aggregated normalized content.
               message::discovery::request::Content operator() () const;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( m_requests);
               )

            private:
               std::vector< content::Request> m_requests;
               
            };
            
         } // pending


         struct Accumulate
         {
            void add( message::discovery::Request message);
            void add( message::discovery::api::Request message);
            void add( message::discovery::topology::direct::Update message);
            void add( message::discovery::topology::implicit::Update message);
            void add( message::discovery::reply::Content lookup);

            accumulate::Requests extract();

            //! @returns true if we should accumulate
            explicit operator bool() const noexcept;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( m_deadline);
               CASUAL_SERIALIZE( m_heuristic);
               CASUAL_SERIALIZE( m_requests);
            )

         private:
            std::optional< common::signal::timer::Deadline> m_deadline;
            accumulate::Heuristic m_heuristic;
            accumulate::Requests m_requests;
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
            common::message::coordinate::minimal::fan::Out< message::discovery::Reply> discovery;
            common::message::coordinate::fan::Out< message::discovery::lookup::Reply, common::process::Handle> lookup;
            common::message::coordinate::fan::Out< message::discovery::fetch::known::Reply, common::process::Handle> known;

            template< typename ID>
            inline void failed( ID&& id) { discovery.failed( id); lookup.failed( id); known.failed( id);}
            
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

         state::Accumulate accumulate;
         state::pending::Content pending_content;
         state::Providers providers;

         void failed( common::strong::process::id pid);
         void failed( const common::strong::ipc::id& ipc);

         bool done() const noexcept;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( coordinate);
            CASUAL_SERIALIZE( multiplex);
            CASUAL_SERIALIZE( service_name);
            CASUAL_SERIALIZE( accumulate);
            CASUAL_SERIALIZE( pending_content);
            CASUAL_SERIALIZE( providers);
         )

      };
   } // domain::discovery:
} // casual