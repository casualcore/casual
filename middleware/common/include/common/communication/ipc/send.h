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
      namespace coordinator
      {
         namespace detail::deduce
         {
            template< typename D>
            constexpr auto& destination( D& destination) noexcept
            {
               if constexpr( std::is_same_v< traits::remove_cvref_t< D>, strong::ipc::id>)
                  return std::as_const( destination);
               else if constexpr( std::is_same_v< traits::remove_cvref_t< D>, process::Handle>)
                  return std::as_const( destination.ipc);
            }
         } // detail::deduce


         struct Message
         {
            using callback_type = common::unique_function< void( const strong::ipc::id&, const ipc::message::Complete&) const>;

            inline Message( ipc::message::complete::Send&& complete)
               : complete{ std::move( complete)}
            {}

            inline Message( ipc::message::complete::Send&& complete, callback_type&& callback)
               : complete{ std::move( complete)}, callback{ std::move( callback)}
            {}

            void error( const strong::ipc::id& destination) const noexcept;

            inline auto type() const noexcept { return complete.type();}

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

            inline Remote( select::Directive& directive, ipc::partial::Destination&& destination, Message&& message)
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

            inline auto& destination() const noexcept { return m_destination;}
            inline auto& queue() const noexcept { return m_queue;}


            inline friend bool operator == ( const Remote& lhs, const strong::ipc::id& rhs) { return lhs.m_destination.ipc() == rhs;}
            inline friend bool operator == ( const Remote& lhs, strong::file::descriptor::id rhs) { return lhs.descriptor() == rhs;}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_destination, "destination");
               CASUAL_SERIALIZE_NAME( m_queue, "queue");
            )

         private:

            ipc::partial::Destination m_destination;
            queue_type m_queue;
         };
         
      } // coordinator


      struct Coordinator
      {
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
         strong::correlation::id send( D&& destination, M&& message, C&&... callback)
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
               return send_dispatch( 
                  destination, 
                  coordinator::Message{ ipc::message::complete::Send{ std::forward< M>( message)}, std::forward< C>( callback)...},
                  traits::priority::tag< 1>{});
            }
            else if constexpr( std::is_same_v< traits::remove_cvref_t< M>, ipc::message::complete::Send>)
            {
               return send_dispatch( 
                  destination, 
                  coordinator::Message{ std::forward< M>( message), std::forward< C>( callback)...},
                  traits::priority::tag< 1>{});
            }
            else
            {
               return send_dispatch( 
                  destination, 
                  coordinator::Message{ ipc::message::complete::Send{ serialize::native::complete< ipc::message::Complete>( std::forward< M>( message))}, std::forward< C>( callback)...},
                  traits::priority::tag< 1>{});
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

         inline auto& destinations() const noexcept { return m_destinations;}

         inline bool empty() const noexcept { return m_destinations.empty();}
         inline operator bool() const noexcept { return ! empty();}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_destinations, "destinations");
         )

      private:

         strong::correlation::id send( const strong::ipc::id& ipc, coordinator::Message&& message);

         //! Take care of devices that can "connect"
         template< typename D>
         auto send_dispatch( D& device, coordinator::Message&& message, traits::priority::tag< 1>) 
            -> decltype( void( device.connector().connect()), strong::correlation::id{})
         {
            //! TODO: this does not feel that good, at all.. The whole "connect" thing should
            //!   be at "one place". Now we we have this copy... Try to consolidate...

            auto result = message.complete.correlation();

            if( auto found = algorithm::find( m_destinations, device.connector().process().ipc))
            {
               if( found->push( std::move( message)).send( *m_directive))
                  m_destinations.erase( std::begin( found));

               return result;
            }
            else
            {
               while( true)
               {
                  try
                  {
                     ipc::partial::Destination destination{ device.connector().process().ipc};
                     if( ! ipc::partial::send( destination, message.complete))
                        m_destinations.emplace_back( *m_directive, std::move( destination), std::move( message));

                     return result;
                  }
                  catch( ...)
                  {
                     auto error = exception::capture();
                     if( error.code() == code::casual::communication_unavailable || error.code() == code::casual::invalid_argument)
                     {
                        // Let connector take a crack at resolving this problem, if implemented...
                        if( ! device.connector().connect())
                           throw;
                     }
                     else if( error.code() == code::casual::interrupted)
                        signal::dispatch();
                     else
                        throw;
                  }
               }

               return result;
            }
         }

         template< typename D>
         strong::correlation::id send_dispatch( D& device, coordinator::Message&& message, traits::priority::tag< 0>)
         {
            return send( coordinator::detail::deduce::destination( device), std::move( message));
         }

         std::vector< coordinator::Remote> m_destinations;
         select::Directive* m_directive = nullptr;
      };
      
   } // common::communication::ipc::send
} // casual