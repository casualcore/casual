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
         void registration();
         void registration( const common::process::Handle& process);

      } // inbound

      using Request = message::discovery::Request;
      using Reply = message::discovery::Reply;

      //! sends an discovery request to casual-domain-discovery, 
      //! that will "ask" all regestrated _inbounds_, and possible outbounds, and accumulate one reply.
      //! @returns the correlation id.
      //! @attention reply will be sent to the process in the request.
      common::Uuid request( const Request& request);
      
      namespace outbound
      {
         using Directive = message::discovery::outbound::Registration::Directive;
         void registration( Directive directive = Directive::regular);
         void registration( const common::process::Handle& process, Directive directive = Directive::regular);

         using Request = message::discovery::outbound::Request;
         using Reply = message::discovery::outbound::Reply;

         //! sends an outbound discovery request to casual-domain-discovery, 
         //! that will "ask" all regestrated _outbounds_, and accumulate one reply.
         //! @returns the correlation id.
         //! @attention reply will be sent to the process in the request.
         common::Uuid request( const Request& request);

         namespace blocking
         {
            void request( const Request& request);
         } // blocking

      } // outbound

      namespace rediscovery
      {
         void registration();

         using Request = message::discovery::rediscovery::Request;
         using Reply = message::discovery::rediscovery::Reply;

         common::Uuid request();

         namespace blocking
         {
            void request();
         } // blocking
         
      } // rediscovery

   } // domain::discovery  
} // casual