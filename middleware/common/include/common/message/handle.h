//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/server.h"
#include "common/message/domain.h"
#include "common/message/dispatch.h"
#include "common/log.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace handle
         {

            //! Replies to a ping message
            struct Ping
            {
               void operator () ( const server::ping::Request& message);
            };

            //! @throws exception::casual::Shutdown if message::shutdown::Request is dispatched
            struct Shutdown
            {
               void operator () ( const message::shutdown::Request& message);
            };

            namespace global
            {
               //! gather _global state_ from the process and reply the information to caller
               struct State 
               {
                  void operator () ( const message::domain::instance::global::state::Request& message);
               };
            } // global
            

            template< typename Device> 
            auto defaults( Device&& device)
            {
               return device.handler( 
                  handle::Ping{},
                  handle::Shutdown{},
                  handle::global::State{});
            }
            
            

            //! Handles and discard a given message type
            template< typename Message> 
            auto discard()
            {
               return []( Message& message)
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

         } // handle
      } // message
   } // common
} // casual
