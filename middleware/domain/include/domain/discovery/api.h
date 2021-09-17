//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "domain/message/discovery.h"

#include "common/process.h"

namespace casual
{
   namespace domain::discovery
   {
      namespace inbound
      {  
         //! register for discovery (to answer an outbound discvery)
         //! @attention will _flush::send_ using ipc::inbound::device()
         //! @{
         void registration();
         void registration( const common::process::Handle& process);
         //! @}

      } // inbound

      using Request = message::discovery::Request;
      using Reply = message::discovery::Reply;
      using correlation_type = common::strong::correlation::id;

      //! sends an discovery request to casual-domain-discovery, 
      //! that will "ask" all regestrated _inbounds_, and possible outbounds, and accumulate one reply.
      //! @returns the correlation id.
      //! @attention reply will be sent to the process in the request.
      //! @attention will _flush::send_ using ipc::inbound::device()
      correlation_type request( const Request& request);
      
      namespace outbound
      {
         using Directive = message::discovery::outbound::Registration::Directive;

         //! register for discovery (can send discovery to others)
         //! @attention will _flush::send_ using ipc::inbound::device()
         //! @{
         void registration( Directive directive = Directive::regular);
         void registration( const common::process::Handle& process, Directive directive = Directive::regular);
         //! @}

         using Request = message::discovery::outbound::Request;
         using Reply = message::discovery::outbound::Reply;

         //! sends an outbound discovery request to casual-domain-discovery, 
         //! that will "ask" all regestrated _outbounds_, and accumulate one reply.
         //! @returns the correlation id.
         //! @attention reply will be sent to the process in the request.
         //! @attention will _flush::send_ using ipc::inbound::device()
         correlation_type request( const Request& request);

      } // outbound

      namespace rediscovery
      {
         //! register for rediscovery
         //! @attention will _flush::send_ using ipc::inbound::device()
         void registration();

         using Request = message::discovery::rediscovery::Request;
         using Reply = message::discovery::rediscovery::Reply;

         correlation_type request();

         namespace blocking
         {
            void request();
         } // blocking
         
      } // rediscovery

   } // domain::discovery  
} // casual