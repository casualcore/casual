//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/ipc.h"

#include "common/message/type.h"
#include "common/message/dispatch.h"
#include "common/execute.h"

namespace casual
{
   namespace common::communication::ipc::reply
   {
      namespace reverse
      {
         namespace detail
         {
            template< typename M>
            auto create_handle( M)
            {
               return []( M& message)
               {
                  auto reply = common::message::reverse::type( message);

                  try
                  {
                     device::blocking::send( message.process.ipc, reply);
                  }
                  catch( ...)
                  {
                     exception::sink();
                  }
               };
            }
         } // detail

         template< typename... Ms>
         auto handler( Ms... messages)
         {
            auto handler = common::message::dispatch::handler( ipc::inbound::device());
            ( ... , handler.insert( detail::create_handle( messages)) );
            
            return handler;
         }
         
      } // reverse

      template< typename H>
      auto guard( ipc::inbound::Device& inbound, H handler)
      {
         return execute::scope( [ &inbound, handler = std::move( handler)]() mutable
         {
            // make sure writers can detect that we won't consume any more.
            inbound.connector().block_writes();

            // consume all messages that writers already has written (before the block_writes above)
            while( handler( device::non::blocking::next( inbound)))
               ; // no-op

         });
      }

      template< typename H>
      auto guard( H handler)
      {
         return guard( ipc::inbound::device(), std::move( handler));
      }
      
   } // common::communication::ipc::reply
} // casual