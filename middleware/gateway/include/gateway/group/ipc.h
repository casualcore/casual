//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/ipc.h"
#include "common/communication/ipc/flush/send.h"
#include "common/communication/instance.h"
#include "common/communication/select.h"

namespace casual
{
   namespace gateway::group::ipc
   {
      namespace manager
      {
         inline auto& service() { return common::communication::instance::outbound::service::manager::device();}
         inline auto& transaction() { return common::communication::instance::outbound::transaction::manager::device();}
         inline auto& gateway() { return common::communication::instance::outbound::gateway::manager::device();}
         namespace optional
         {
            inline auto& queue() { return common::communication::instance::outbound::queue::manager::optional::device();}
         } // optional

      } // manager

      inline auto& inbound() { return common::communication::ipc::inbound::device();}

      namespace flush
      {
         using namespace common::communication::ipc::flush;

      } // flush


      namespace handle::dispatch
      {
         template< typename Policy, typename State, typename Handler>
         auto create( State& state, Handler handler)
         {
            return [ &state, handler = std::move( handler)]( common::strong::file::descriptor::id fd, common::communication::select::tag::read) mutable
            {
               auto descriptor = common::strong::ipc::descriptor::id{ fd};

               if( auto inbound = state.connections.find_internal( descriptor))
               {
                  try
                  {
                     auto count = Policy::next::tcp();

                     while( count-- > 0 && handler( common::communication::device::non::blocking::next( *inbound), descriptor))
                        ; // no-op
                  }
                  catch( ...)
                  {
                     // TODO What do we do here? We just let i propagate for now...
                     throw;
                  }
                  return true;
               }
               return false;
            };
         }
      } // handle::dispatch

   } // gateway::group::ipc
} // casual