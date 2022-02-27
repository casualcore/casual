//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/message/event.h"
#include "common/communication/ipc/message.h"
#include "common/serialize/native/complete.h"
#include "common/exception/capture.h"
#include "common/code/raise.h"
#include "common/string.h"

#include <string>

namespace casual
{
   namespace common::event
   {
      namespace detail
      {
         
         void send( communication::ipc::message::Complete&& message);

         namespace error
         {
            using Severity = message::event::Error::Severity;
            void send( std::error_code code, Severity severity, std::string message);            
         } // error

      } // detail

      template< typename Event>
      constexpr void send( Event&& event)
      {
         static_assert( message::is::event::message< std::decay_t< Event>>(), "only events can be sent");
         detail::send( serialize::native::complete< communication::ipc::message::Complete>( std::forward< Event>( event)));
      }

      namespace notification
      {
         template< typename... Ts>
         auto send( Ts&&... ts)
         {
            message::event::Notification event{ process::handle()};
            event.message = string::compose( std::forward< Ts>( ts)...);
            event::send( event);
         }
         
      } // notification

      namespace error
      {

         //! Sends an error event (with severity error) to domain manager, that will forward the event
         //! to possible listeners
         template< typename Code, typename... Ts>
         auto send( Code code, Ts&&... ts)
         {
            detail::error::send( code, detail::error::Severity::error, string::compose( std::forward< Ts>( ts)...));
         }

         namespace fatal
         {
            //! Sends an error event (with severity fatal) to domain manager, that will forward the event
            //! to possible listeners, and shutdown the domain!
            template< typename Code, typename... Ts>
            auto send( Code code, Ts&&... ts)
            {
               detail::error::send( code, detail::error::Severity::fatal, string::compose( std::forward< Ts>( ts)...));
            }
            
         } // fatal


         //! Sends an error event (with severity error) to domain manager, that will forward the event
         //! to possible listeners
         //! then: raise the code
         template< typename Code, typename... Ts>
         [[noreturn]] void raise( Code code, Ts&&... ts)
         {
            auto message = string::compose( std::forward< Ts>( ts)...);
            detail::error::send( code, detail::error::Severity::error, message);
            code::raise::error( code, message);
         }

         namespace fatal
         {
            //! Sends an error event (with severity fatal) to domain manager, that will forward the event
            //! to possible listeners, and shutdown the domain!
            //! then: raise the code
            template< typename Code, typename... Ts>
            [[noreturn]] void raise( Code code, Ts&&... ts)
            {
               auto message = string::compose( std::forward< Ts>( ts)...);
               detail::error::send( code, detail::error::Severity::fatal, message);
               code::raise::error( code, message);
            }
            
         } // fatal


      } // error

      namespace guard
      {
         //! sends a fatal error if an exception is thrown
         template< typename F, typename... Ts>
         auto fatal( F&& functor, Ts&&... ts)
         {
            try
            {
               return functor( std::forward< Ts>( ts)...);
            }
            catch( ...)
            {
               auto error = exception::capture();
               event::error::fatal::send( error.code(), error.what());
               throw error;
            }
         }
      } // guard

   } // common::event
} // casual


