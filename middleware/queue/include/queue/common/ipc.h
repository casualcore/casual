//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/instance.h"
#include "common/communication/ipc.h"

namespace casual
{
   namespace queue::ipc
   {

      inline auto& device() { return common::communication::ipc::inbound::device();}

      namespace queue
      {
         inline auto& manager() { return common::communication::instance::outbound::queue::manager::device();}   
      } // queue

      namespace service
      {
         inline auto& manager() { return common::communication::instance::outbound::service::manager::device();}
      } // service

      namespace transaction
      {
         inline auto& manager() { return common::communication::instance::outbound::transaction::manager::device();}
      } // transaction

      namespace gateway::optional
      {
         inline auto& manager() { return common::communication::instance::outbound::gateway::manager::optional::device();}
      } // gateway::optional


      //! since we're sending a lot of messages in the same 'batch', we need to flush our inbound
      //! to mitigate 'deadlocks' 
      namespace flush
      {
         //! flushes inbound if non blocking send fails
         template< typename D, typename M>
         auto send( D&& destination, M&& message)
         {
            auto result = common::communication::device::non::blocking::send( destination, message);
            while( ! result)
            {
               ipc::device().flush();
               result = common::communication::device::non::blocking::send( destination, message);
            }
            return result;
         }

         namespace optional
         {
            //! flushes inbound before blocking send
            template< typename D, typename M>
            auto send( D&& destination, M&& message) -> decltype( flush::send( destination, message))
            {
               try 
               {
                  return flush::send( destination, std::forward< M>( message));
               }
               catch( ...)
               {
                  if( common::exception::error().code() != common::code::casual::communication_unavailable)
                     throw;

                  common::log::line( common::communication::log, common::code::casual::communication_unavailable, " failed to send message - action: ignore");
                     return {};
               }
            }
         } // optional
      } // flush

   } // queue::ipc
} // casual