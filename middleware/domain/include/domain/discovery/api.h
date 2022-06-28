//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "domain/message/discovery.h"

#include "common/process.h"
#include "common/communication/ipc/send.h"

namespace casual
{
   namespace domain::discovery
   {
      using Send = common::communication::ipc::send::Coordinator;

      using Request = message::discovery::Request;
      using Reply = message::discovery::Reply;

      //! Used by _inbounds_ that get's this message from another domain's outbound.
      //! Will "ask" all regestrated _internals_, and possible _outbounds_, and accumulate one reply.
      //! @returns the correlation id.
      //! @attention reply will be sent to the process in the request.
      //! @attention will _flush::send_ using ipc::inbound::device()
      common::strong::correlation::id request( const Request& request);

      common::strong::correlation::id request( Send& multiplex, const Request& request);

      namespace provider
      {
         using Ability = message::discovery::api::provider::registration::Ability;
         void registration( common::Flags< Ability> abilities);
         void registration( const common::process::Handle& process, common::Flags< Ability> abilities);
      } // provider

      common::strong::correlation::id request(
         std::vector< std::string> services, 
         std::vector< std::string> queues, 
         common::strong::correlation::id correlation = common::strong::correlation::id::generate());
      

      common::strong::correlation::id request(
         Send& multiplex,
         std::vector< std::string> services, 
         std::vector< std::string> queues, 
         common::strong::correlation::id correlation = common::strong::correlation::id::generate());

      namespace topology
      {
         namespace direct
         {
            void update( Send& multiplex);
            void update( Send& multiplex, const message::discovery::topology::direct::Update& message);

         } // direct

         namespace implicit
         {
            void update( Send& multiplex, const message::discovery::topology::implicit::Update& message);
         } // implicit
      } // topology
      
      namespace rediscovery
      {
         using Reply = message::discovery::api::rediscovery::Reply;

         common::strong::correlation::id request();   
      } // rediscovery
      

   } // domain::discovery  
} // casual