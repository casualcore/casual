//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/event/send.h"


#include "common/communication/ipc.h"
#include "common/signal.h"


namespace casual
{
   namespace common
   {
      namespace event
      {

         namespace error
         {
            void send( std::string message, Severity severity)
            {
               Trace trace{ "common::domain::event::error::send"};

               //
               // We block all signals but SIG_INT
               //
               signal::thread::scope::Mask block{ signal::set::filled( signal::Type::interrupt)};

               message::event::domain::Error error;
               error.message = std::move( message);
               error.severity = severity;
               error.executable = process::basename();
               error.pid = process::id();

               communication::ipc::blocking::send( communication::ipc::domain::manager::device(), error);
            }

         } // error
      } // event
   } // common
} // casual
