//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "http/code.h"

#include "common/service/lookup.h"
#include "common/serialize/macro.h"
#include "common/service/header.h"


#include <vector>
#include <string>
#include <memory>
#include <optional>

namespace casual
{
   namespace http::inbound::call
   {  
      namespace header
      {
         using Field = common::service::header::Field;
      } // header

      struct Payload
      {
         std::vector< header::Field> header;
         std::vector< char> body;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( header);
            CASUAL_SERIALIZE( body);
         )
      };

      struct Arguments
      {
         std::string service;
         Payload payload;
      };

      struct Reply
      {
         http::code code;
         Payload payload;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( code);
            CASUAL_SERIALIZE( payload);
         )
      };

      //! the call context that holds the state machine for a service call
      //! 
      //! possible initialization: `nginx_context.user_data = new plugin::call::Context{ std::move( arguments)};
      struct Context
      {
         Context( Arguments arguments);

         //! will take care of cancelling necessary stuff, if any.
         ~Context();

         Context( Context&&);
         Context& operator = ( Context&&);

         //! @returns the reply if ready, absent if not.
         //! @post if a reply is returned then this context is 'consumed', and the only valid operation
         //!   is calling the destructor.
         std::optional< Reply> receive() noexcept;
         
         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_payload, "payload");
            CASUAL_SERIALIZE_NAME( m_lookup, "lookup");
            CASUAL_SERIALIZE_NAME( m_correlation, "correlation");
         )
      
      private:
         Payload m_payload;
         std::optional< common::service::non::blocking::Lookup> m_lookup;
         common::strong::correlation::id m_correlation;
      };



   } // http::inbound::call
} // casual