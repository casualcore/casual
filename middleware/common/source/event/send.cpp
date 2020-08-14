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
   namespace common
   {
      namespace event
      {
         namespace detail
         {
            void send( communication::message::Complete&& complete)
            {
               Trace trace{ "common::domain::event::detail::send"};

               // We block all signals but SIG_INT
               signal::thread::scope::Mask block{ signal::set::filled( code::signal::interrupt)};

               communication::device::blocking::optional::put( communication::instance::outbound::domain::manager::device(), complete);
            }

            void send( std::error_code code, Severity severity, std::string message)
            {
               Trace trace{ "common::domain::event::error::send"};

               auto stream = []( auto severity) -> std::ostream&
               {
                  if( severity == Severity::warning)
                     return log::category::warning;
                  return log::category::error;
               };

               log::line( stream( severity), code, ' ', message);

               // We block all signals but SIG_INT
               signal::thread::scope::Mask block{ signal::set::filled( code::signal::interrupt)};

               message::event::Error error{ process::handle()};
               error.code = code;
               error.message = std::move( message);
               error.severity = severity;

               communication::device::blocking::send( communication::instance::outbound::domain::manager::device(), error);
            }
         } // detail


      } // event
   } // common
} // casual
