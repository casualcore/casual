//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/server.h"
#include "common/message/domain.h"
#include "common/message/internal.h"
#include "common/message/dispatch.h"
#include "common/communication/ipc.h"
#include "common/log.h"


namespace casual
{
   namespace common::message::dispatch::handle
   {
      namespace dump
      {
         //! dumps the state to casual log.
         template< typename State>
         auto state( const State& state) noexcept
         {
            return [&state]( const message::internal::dump::State&)
            {
               std::ostringstream out;
               log::write( out, "state: ", state);
               log::stream::write( "casual.internal", std::move( out).str());
            };
         }
      } // dump

      using handler_type = decltype( message::dispatch::handler( communication::ipc::inbound::device()));

      //! @return a handler with all the default handlers:
      //! * Ping - Replies to a ping message
      //! * Shutdown - throws exception::casual::Shutdown if message::shutdown::Request is dispatched
      //! * global::State gather - global state_ from the process and reply the information to caller
      handler_type defaults() noexcept;

      //! @returns all defaults + state dump.
      template< typename State>
      handler_type defaults( const State& state) noexcept
      {
         auto result = defaults();
         result.insert( dump::state( state));
         return result;
      }

      //! Handles and discard a given message type
      template< typename Message> 
      auto discard()
      {
         return []( const Message& message)
         {
            log::line( log::debug, "discard message: ", message);
         };
      }

      //! Dispatch and assigns a given message
      template< typename M>
      auto assign( M& target)
      {
         return [&target]( M& message)
         {
            target = message;
         };
      }


      namespace protocol
      {
         namespace detail
         {
            template< typename M, typename H>
            auto create( H handler)
            {
               return [ handler = std::move( handler)]( M& message)
               {
                  handler( message);
               };
            }

         } // detail

         //! helper to 'define' the _protocol_ which is the same as what complete type
         //! of the logical message should be used (for de(serialization) and stuff)
         template< typename Complete>
         struct basic
         {
            using dispatch_type = dispatch::basic_handler< Complete>;

            //! @returns dispatch_type that holds the provided handlers.
            template< typename... Ts>
            static auto create( Ts&&... ts)
            {
               return dispatch_type{ std::forward< Ts>( ts)...};
            }

            //! composes a _dispatch handler_ for [2..*] message types, that uses the 
            //! the same generic `handler`. Useful if one wants to handle several different
            //! messages in exactly the same way.
            //! @note  `handler` is copied to the actual handlers.
            //! @returns `dispatch_type` that holds the handlers for each message.  
            template< typename... Ms, typename H>
            static auto compose( H handler)
            {
               return ( ... + dispatch_type{ detail::create< Ms>( handler)} );
            }
         };

         using ipc = basic< std::remove_cvref_t< decltype( communication::ipc::inbound::device())>::complete_type>;
      } // protocol

   } // common::message::dispatch::handle
} // casual
