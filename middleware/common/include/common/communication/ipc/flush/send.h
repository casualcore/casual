//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/ipc.h"
#include "common/exception/capture.h"

namespace casual
{
   namespace common::communication::ipc::flush
   {
      //! flushes `inbound` until non blocking send success
      template< typename D, typename M>
      auto send( inbound::Device& inbound, D&& destination, M&& message)
      {
         while( true)
         {
            if( auto result = device::non::blocking::send( destination, message))
               return result;

            inbound.flush();
         }
      }

      //! flushes 'global' inbound until non blocking send success
      template< typename D, typename M>
      auto send( D&& destination, M&& message)
      {
         return send( inbound::device(), std::forward< D>( destination), std::forward< M>( message));
      }

      namespace optional
      {
         //! flushes `inbound` until non blocking send success. If `destination` is unavailable
         //! 'empty' correlation is returned
         template< typename D, typename M>
         auto send( inbound::Device& inbound, D&& destination, M&& message) -> decltype( flush::send( destination, message))
         {
            try 
            {
               return flush::send( inbound, std::forward< D>( destination), std::forward< M>( message));
            }
            catch( ...)
            {
               if( exception::capture().code() != code::casual::communication_unavailable)
                  throw;

               log::line( communication::log, code::casual::communication_unavailable, " failed to send message - action: ignore");

               return {};
            }
         }

         //! flushes 'global' inbound until non blocking send success. If `destination` is unavailable
         //! 'empty' correlation is returned
         template< typename D, typename M>
         auto send( D&& destination, M&& message) -> decltype( flush::send( destination, message))
         {
            return optional::send( ipc::inbound::device(),  std::forward< D>( destination), std::forward< M>( message));
         }
         
      } // optional
   } // common::communication::ipc::flush
} // casual