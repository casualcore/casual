//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/ipc.h"
#include "common/communication/select.h"
#include "common/strong/id.h"
#include "common/serialize/native/complete.h"

namespace casual
{
   namespace common::communication::ipc::send
   {
      namespace detail::coordinator::deduce
      {
         template< typename D>
         constexpr auto& destination( D& destination) noexcept
         {
            if constexpr( std::is_same_v< traits::remove_cvref_t< D>, strong::ipc::id>)
               return std::as_const( destination);
            else if constexpr( std::is_same_v< traits::remove_cvref_t< D>, process::Handle>)
               return std::as_const( destination.ipc);
            else
               return std::as_const( destination.connector().process().ipc);
         }
      } // detail::coordinator::deduce

      struct Coordinator
      {
         using callback_type = common::unique_function< void( const strong::ipc::id&, ipc::message::complete::Send&)>;

         inline Coordinator( select::Directive& directive) : m_directive{ &directive} {}
         
         //! Tries to send a message to the destination ipc, and an optional callback that will be
         //! invoked if the destination is _lost_
         //!   * If there are pending messages to the ipc, the message (complete) will be
         //!     pushed back in the queue, and each message in the queue is tried to send, until one "fails"
         //!   * If there are no messages pending for the ipc, the message will be
         //!     tried to send directly. If not successful (the whole complete message is sent), 
         //!     the message ( possible partial message) will be pushed back in the queue
         //! @returns the correlation for the message, regardless if the message was sent directly or not.
         //! @{
         template< typename D, typename M, typename... C>
         strong::correlation::id send( D& destination, M&& message, C&&... callback)
         {
            static_assert( sizeof...(C) <= 1, "[0..1] callbacks required");

            // Destination can be:
            //   * an ipc id
            //   * an outbound device 
            // The message can be:
            //   * a 'regular' Complete message
            //   * a complete::Send message
            //   * a structured message that needs to be serialized
            // We need to dispatch on these 6 permutations, hence we deduce the destination
            // separately, and constexpr if on the message type. 


            if constexpr( std::is_same_v< traits::remove_cvref_t< M>, ipc::message::Complete>)
            {
               return send( 
                  detail::coordinator::deduce::destination( destination), 
                  Message{ ipc::message::complete::Send{ std::forward< M>( message)}, std::forward< C>( callback)...});
            }
            else if constexpr( std::is_same_v< traits::remove_cvref_t< M>, ipc::message::complete::Send>)
            {
               return send( 
                  detail::coordinator::deduce::destination( destination), 
                  Message{ std::forward< M>( message), std::forward< C>( callback)...});
            }
            else
            {
               return send( 
                  detail::coordinator::deduce::destination( destination), 
                  Message{ ipc::message::complete::Send{ serialize::native::complete< ipc::message::Complete>( std::forward< M>( message))}, std::forward< C>( callback)...});
            }
         }
         //! @}

         //! Function operator to be used with communication::select:dispatch::pump for writes.
         //! @returns true if the descriptor was found (to guide the 'pump' that it will not be 
         //!   needed to try (possible) other _dispatchers_)
         bool operator () ( strong::file::descriptor::id descriptor, communication::select::tag::write) &;

         //! try to send all pending messages, if any.
         //! @return `empty()`
         bool send() noexcept;

         //! @return the accumulated count of all pending messages
         platform::size::type pending() const noexcept;

         inline bool empty() const noexcept { return m_destinations.empty();}
         inline operator bool() const noexcept { return ! empty();}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_destinations, "destinations");
         )

      private:

         struct Message
         {
            inline Message( ipc::message::complete::Send&& complete)
               : complete{ std::move( complete)}
            {}

            inline Message( ipc::message::complete::Send&& complete, callback_type&& callback)
               : complete{ std::move( complete)}, callback{ std::move( callback)}
            {}

            inline void error( const strong::ipc::id& destination)
            {
               if( callback)
                  callback( destination, complete);
            }

            ipc::message::complete::Send complete;
            callback_type callback;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( complete);
               CASUAL_SERIALIZE_NAME( predicate::boolean( callback), "callback");
            )
         };

         struct Remote
         {
            using queue_type = std::deque< Message>;

            inline Remote( select::Directive& directive, ipc::outbound::partial::Destination&& destination, Message&& message)
               : m_destination{ std::move( destination)}
            {
               m_queue.emplace_back( std::move( message));
               directive.write.add( descriptor());
            }

            strong::file::descriptor::id descriptor() const noexcept { return m_destination.socket().descriptor();}

            bool send( select::Directive& directive) noexcept;

            inline Remote& push( Message&& message)
            {
               m_queue.push_back( std::move( message));
               return *this;
            }

            inline platform::size::type size() const noexcept { return m_queue.size();}


            inline friend bool operator == ( const Remote& lhs, const strong::ipc::id& rhs) { return lhs.m_destination.ipc() == rhs;}
            inline friend bool operator == ( const Remote& lhs, strong::file::descriptor::id rhs) { return lhs.descriptor() == rhs;}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_destination, "destination");
               CASUAL_SERIALIZE_NAME( m_queue, "queue");
            )

         private:

            ipc::outbound::partial::Destination m_destination;
            queue_type m_queue;
         };

         strong::correlation::id send( const strong::ipc::id& ipc, Message&& message);

         std::vector< Remote> m_destinations;

         select::Directive* m_directive = nullptr;
      };
      
   } // common::communication::ipc::send
} // casual