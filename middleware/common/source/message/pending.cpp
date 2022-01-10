//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/message/pending.h"

#include "common/communication/ipc.h"

namespace casual
{
   namespace common::message::pending
   {

      bool Message::sent() const
      {
         return destinations.empty();
      }

      Message::operator bool () const
      {
         return sent();
      }

      void Message::remove( strong::ipc::id ipc)
      {
         algorithm::container::trim( destinations, algorithm::remove( destinations, ipc));
      }

      void Message::remove( strong::process::id pid)
      {
         algorithm::container::trim( destinations, algorithm::remove( destinations, pid));
      }

      namespace non
      {
         namespace blocking
         {
            bool send( Message& message)
            {
               auto send = [&]( const common::process::Handle& process)
               {
                  try
                  {
                     return predicate::boolean( communication::device::non::blocking::send( process.ipc, message.complete));
                  }
                  catch( ...)
                  {
                     if( exception::capture().code() != code::casual::communication_unavailable)
                        throw;
   
                     return true;
               }
               };

               algorithm::container::trim( message.destinations, algorithm::remove_if( message.destinations, send));
            
               return message.sent();
            }

         } // blocking
      } // non

   } // common::message::pending
} // casual
