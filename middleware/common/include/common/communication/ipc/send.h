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
            else
               return std::as_const( destination.connector().process().ipc);
         }
      } // detail::coordinator::deduce

      struct Coordinator
      {
         inline Coordinator( select::Directive& directive) : m_directive{ &directive} {}
         
         //! Tries to send a message to the destination ipc
         //!   * If there are pending messages to the ipc, the message (complete) will be
         //!     pushed back in the queue, and each message in the queue is tried to send, until one "fails"
         //!   * If there are no messages pendinging for the ipc, the message will be
         //!     tried to send directly. If not successfull (the whole complete message is sent), 
         //!     the message ( possible partial message) will be pushed back in the queue
         //! @returns the correlation for the message, regardless if the message was sent directly or not.
         //! @{
         template< typename D, typename M>
         strong::correlation::id send( D& destination, M&& message)
         {
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
               return send( detail::coordinator::deduce::destination( destination), 
                  ipc::message::complete::Send{ std::forward< M>( message)});
            else if constexpr( std::is_same_v< traits::remove_cvref_t< M>, ipc::message::complete::Send>)
               return send( detail::coordinator::deduce::destination( destination), std::forward< M>( message));
            else
               return send( detail::coordinator::deduce::destination( destination), 
                  ipc::message::complete::Send{ serialize::native::complete< ipc::message::Complete>( std::forward< M>( message))});
         }

         strong::correlation::id send( const strong::ipc::id& ipc, ipc::message::complete::Send&& complete);
         //! @}

         //! Function operator to be used with communication::select:dispatch::pump for writes.
         //! @returns true if the descriptor was found (to guide the 'pump' that it will not be 
         //!   needed to try (possible) other _dispatchers_)
         bool operator () ( strong::file::descriptor::id descriptor, communication::select::tag::write) &;

         inline bool empty() const noexcept { return m_destinations.empty();}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_destinations, "destinations");
         )


      private:
         struct Remote
         {
            using queue_type = std::deque< ipc::message::complete::Send>;

            inline Remote( select::Directive& directive, ipc::outbound::partial::Destination&& destination, ipc::message::complete::Send&& complete)
               : m_destination{ std::move( destination)}, m_queue{ algorithm::container::emplace::initialize< queue_type>( std::move( complete))}
            {
               directive.write.add( descriptor());
            }

            strong::file::descriptor::id descriptor() const noexcept { return m_destination.socket().descriptor();}

            bool send( select::Directive& directive);

            inline Remote& push( ipc::message::complete::Send&& complete)
            {
               m_queue.push_back( std::move( complete));
               return *this;
            }


            inline friend bool operator == ( const Remote& lhs, const strong::ipc::id& rhs) { return lhs.m_destination.ipc() == rhs;}
            inline friend bool operator == ( const Remote& lhs, strong::file::descriptor::id rhs) { return lhs.descriptor() == rhs;}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_destination, "destination");
               CASUAL_SERIALIZE_NAME( m_ipc, "ipc");
               CASUAL_SERIALIZE_NAME( m_queue, "queue");
            )

         private:

            ipc::outbound::partial::Destination m_destination;
            strong::ipc::id m_ipc;
            queue_type m_queue;
         };

         std::vector< Remote> m_destinations;

         select::Directive* m_directive = nullptr;
      };
      
   } // common::communication::ipc::send
} // casual