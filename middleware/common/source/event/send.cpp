//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/event/send.h"


#include "common/communication/instance.h"
#include "common/signal.h"


namespace casual
{
   namespace common::event
   {
      namespace detail
      {
         void send( communication::ipc::message::Complete&& complete)
         {
            Trace trace{ "common::domain::event::detail::send"};

            // We block all signals but SIG_INT
            signal::thread::scope::Mask block{ signal::set::filled( code::signal::interrupt)};

            communication::device::blocking::optional::send( communication::instance::outbound::domain::manager::device(), complete);
         }

         namespace error
         {
            void send( std::error_code code, Severity severity, std::string message)
            {
               Trace trace{ "common::domain::event::detail::error::send"};

               log::line( log::category::error, code, ' ', message);

               // We block all signals but SIG_INT
               signal::thread::scope::Mask block{ signal::set::filled( code::signal::interrupt)};

               message::event::Error error{ process::handle()};
               error.code = code;
               error.message = std::move( message);
               error.severity = severity;

               communication::device::blocking::send( communication::instance::outbound::domain::manager::device(), error);
            }
         } // error

      } // detail

   } // common::event
} // casual
