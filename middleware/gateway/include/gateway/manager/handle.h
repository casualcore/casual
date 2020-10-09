//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "gateway/manager/state.h"
#include "gateway/message.h"
#include "gateway/manager/admin/model.h"

#include "common/message/dispatch.h"
#include "common/message/domain.h"
#include "common/message/gateway.h"
#include "common/message/coordinate.h"

#include "common/message/event.h"

#include "common/communication/ipc.h"

namespace casual
{
   namespace gateway
   {
      namespace manager
      {
         namespace ipc
         {
            common::communication::ipc::inbound::Device& device();
         } // ipc

         namespace handle
         {
            using dispatch_type = decltype( common::message::dispatch::handler( ipc::device()));

            void shutdown( State& state);

            void boot( State& state);

            common::Uuid rediscover( State& state);

            namespace process
            {
               void exit( const common::process::lifetime::Exit& exit);
            } // process

            namespace listen
            {
               struct Accept
               {
                  inline Accept( State& state) : m_state( state) {}

                  using descriptor_type = common::strong::file::descriptor::id;

                  void read( descriptor_type descriptor);
                  const std::vector< descriptor_type>& descriptors() const;
               private:
                  std::reference_wrapper< State> m_state;
               };
            } // listen
         } // handle

         handle::dispatch_type handler( State& state);

      } // manager
   } // gateway
} // casual

