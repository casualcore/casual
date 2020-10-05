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
   namespace queue
   {
      namespace ipc
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

         //! since we're sending a lot of messages at the same 'batch', we need to flush our inbound
         //! to mitigate 'deadlocks' 
         namespace flush
         {
            //! flushes inbound before blocking send
            template< typename D, typename M>
            auto send( D&& destination, M&& message)
            {
               ipc::device().flush();
               return common::communication::device::blocking::send( destination, std::forward< M>( message));
            }

            namespace optional
            {
               //! flushes inbound before blocking send
               template< typename D, typename M>
               auto send( D&& destination, M&& message)
               {
                  ipc::device().flush();
                  return common::communication::device::blocking::optional::send( destination, std::forward< M>( message));
               }
            } // optional
         } // flush

      } // ipc

   } // queue
} // casual