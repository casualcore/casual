//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/message/event.h"
#include "common/serialize/native/complete.h"

#include <string>

namespace casual
{
   namespace common
   {
      namespace event
      {
         namespace detail
         {
            void send( communication::message::Complete&& message);
         } // detail

         template< typename Event>
         constexpr void send( Event&& event)
         {
            static_assert( message::is::event::message< Event>(), "only events can be sent");
            detail::send( serialize::native::complete( std::forward< Event>( event)));
         }

         namespace error
         {
            using Severity = message::event::Error::Severity;

            //! Sends an error event to the domain manager, that will forward the event
            //! to possible listeners
            //! @param message
            void send( std::string message, Severity severity = Severity::error);

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
               catch( std::exception& exception)
               {
                  event::error::send( exception.what(), event::error::Severity::fatal);
                  throw;
               }
               catch( ...)
               {
                  event::error::send( "unknown exception - file a bug report", event::error::Severity::fatal);
                  throw;
               }
            }
         } // guard

      } // event
   } // common
} // casual


