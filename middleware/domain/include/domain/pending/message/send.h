//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "domain/common.h"

#include "common/message/pending.h"
#include "common/communication/ipc/message.h"
#include "common/communication/ipc.h"

namespace casual
{
   namespace domain
   {
      namespace pending
      {
         namespace message
         {
            void send( const common::message::pending::Message& message);

            template< typename M>
            auto send( const common::process::Handle& destination, M&& message)
            {
               auto pending = common::message::pending::Message{ std::forward< M>( message), destination};
               message::send( pending);
               return pending.complete.correlation();
            }

            namespace eventually
            {
               namespace detail
               {

                  template< typename D, typename M>
                  auto send( D&& destination, M&& message, common::traits::priority::tag< 1>)
                     -> decltype( message::send( destination.connector().process(), message))
                  {
                     if( auto correlation = common::communication::device::non::blocking::send( destination, message))
                        return correlation;
                           
                     return message::send( destination.connector().process(), std::forward< M>( message));
                  }

                  template< typename D, typename M>
                  auto send( D&& destination, M&& message, common::traits::priority::tag< 0>)
                     -> decltype( message::send( destination, message))
                  {
                     if( auto correlation = common::communication::device::non::blocking::send( destination.ipc, message))
                        return correlation;
                           
                     return message::send( destination, std::forward< M>( message));
                  }
                  
               } // detail

               //! Tries to send the message to destination - non blocking. 
               //! if not successfull send to pending message.
               //! @returns correlation id
               template< typename D, typename M>
               auto send( D&& destination, M&& message) 
                  -> decltype( detail::send( destination, std::forward< M>( message), common::traits::priority::tag< 1>{}))
               {
                  return detail::send( destination, std::forward< M>( message), common::traits::priority::tag< 1>{});
               }
            } // eventually
         } // message
         
      } // pending
   } // domain
} // casual