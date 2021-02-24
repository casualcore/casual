//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/pending/message/send.h"
#include "domain/pending/message/instance.h"
#include "domain/pending/message/message.h"
#include "domain/common.h"

#include "common/communication/instance.h"

namespace casual
{
   using namespace common;

   namespace domain::pending::message
   {
      namespace local
      {
         namespace
         {
            auto& device() 
            {
               static communication::instance::outbound::detail::Device device{ pending::message::instance::identity};
               return device;  
            }
            
         } // <unnamed>
      } // local

      void send( const common::message::pending::Message& message)
      {
         Trace trace{ "domain::pending::message::send"};
         log::line( verbose::log, "message: ", message);

         communication::device::blocking::send( local::device(), caller::Request{ message});
      }

   } // domain::pending::message
} // casual