//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/dispatch.h"
#include "common/communication/ipc/message.h"
#include "common/terminal.h"
#include "common/message/service.h"

#include "service/protocol/call.h"


#include <string_view>
#include <optional>

namespace casual
{
   namespace cli::call
   {
      using handler_type = common::message::dispatch::basic_handler< common::communication::ipc::message::Complete>;

      //! helper to call a service and handle provided events while waiting for the reply
      //! if `R` is not void, the return type is an `std::optional< R>` where `R` is the type of the result
      //! If user has provided `--block false` the return is std::nullopt
      template< typename R, typename... Args>
      auto concurrent( std::string_view service, handler_type event_handler, const Args&... arguments)
      {
         // if no-block we don't mess with events
         if( ! common::terminal::output::directive().block())
         {
            auto complement = casual::service::protocol::binary::Send::Complement{
               .flags = casual::service::send::Flag::no_reply
            };

            casual::service::protocol::binary::Send send;
            send( service, complement, arguments...);

            if constexpr( std::is_void_v< R>)
               return;
            else
               return std::optional< R>{};
         }
            
         auto unsubscribe_guard = common::event::scope::subscribe( event_handler.types());

         bool done = false;

         auto handler = handler_type{
            [ &done]( common::message::service::call::Reply& reply)
            {
               // push it back, we'll block and get it from outside the message loop
               common::communication::ipc::inbound::device().push( std::move( reply));
               done = true;
            }};

         casual::service::protocol::binary::Send send;
         auto receive = send( service, arguments...);

         auto condition = common::event::condition::compose(
            common::event::condition::done( [&done]()
            { 
               return done;
            })
         );

         // listen for events and the reply
         common::message::dispatch::pump( condition,
            std::move( event_handler) + std::move( handler),
            common::communication::ipc::inbound::device());

         // get the reply
         auto result = receive();

         if constexpr( std::is_void_v< R>)
            return;
         else
            return std::optional< R>{ result.template extract< R>()};
      }

      //! helper to call a service
      //! if `R` is not void, the return type is an `std::optional< R>` where `R` is the type of the result
      //! If user has provided `--block false` the return is std::nullopt
      template< typename R, typename... Args>
      auto concurrent( std::string_view service, const Args&... arguments)
      {
         // if no-block we don't mess with events
         if( ! common::terminal::output::directive().block())
         {
            auto complement = casual::service::protocol::binary::Send::Complement{
               .flags = casual::service::send::Flag::no_reply
            };

            casual::service::protocol::binary::Send send;
            send( service, complement, arguments...);

            if constexpr( std::is_void_v< R>)
               return;
            else
               return std::optional< R>{};
         }

         casual::service::protocol::binary::Call call;
         auto result = call( service, arguments...);

         if constexpr( std::is_void_v< R>)
            return;
         else
            return std::optional< R>{ result.template extract< R>()};
      }

      template< typename R, typename... Args>
      auto sequential( std::string_view service, const Args&... arguments)
      {
         casual::service::protocol::binary::Call call;
         auto result = call( service, arguments...);

         if constexpr( std::is_void_v< R>)
            return;
         else
            return result.template extract< R>();
      }
      
   } // cli::call
   
} // casual
