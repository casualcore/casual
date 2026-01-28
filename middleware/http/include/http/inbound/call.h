//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "http/code.h"

#include "common/serialize/macro.h"
#include "common/strong/id.h"
#include "casual/header.h"

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
         using Field = casual::header::Field;
      } // header

      namespace detail
      {
         namespace transform
         {
            //! tries to transform execution and parent span from the `value`
            // `value` is expected to be in the format of a traceparent header value
            //! @note exposed for testing purposes
            auto span( std::string_view value) -> std::tuple< common::strong::execution::id, common::strong::execution::span::id>;
         } // transform
         
      } // detail

      struct Payload
      {
         std::vector< header::Field> header;
         platform::binary::type body;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( header);
            CASUAL_SERIALIZE( body);
         )
      };



      //! assumes that we can 'extract' the full url from nginx.
      //! should this type have the responsibility to extract query
      //! parameters and such? 
      struct url : std::string
      {
         using std::string::string;
      };

      enum struct Directive : short
      {
         //! transform information in the _Request_ and do a service call
         service = 0,
         //! forward the http request 'un-modified' to callee.
         forward = 1
      };
      std::ostream& operator << ( std::ostream& out, Directive value);


      struct Request
      {
         std::string service;

         //! GET, POST/PUT, DELETE, etc
         std::string method;

         std::string request_line; // the first line of the request, e.g. "GET / HTTP/1.1"

         http::inbound::call::url url;

         Payload payload;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( service);
            CASUAL_SERIALIZE( method);
            CASUAL_SERIALIZE( request_line);
            CASUAL_SERIALIZE( url);
            CASUAL_SERIALIZE( payload);
         )
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
      //! possible initialization: `nginx_context->casual_call_context = http::inbound::call::Context{ http::inbound::call::Directive::service, std::move( request)};
      struct Context
      {
         Context() = default;

         //! only Directive::service is implemented for now...
         Context( Directive directive, Request request);

         //! will take care of cancelling necessary stuff, if any.
         ~Context();

         Context( Context&&);
         Context& operator = ( Context&&);

         //! @returns the reply if ready, absent if not.
         //! @post if a reply is returned then this context is 'consumed', and the only valid operation
         //!   is calling the destructor.
         //! example:  
         //! if( auto reply = context.receive()) 
         //! {
         //!    // do stuff with reply
         //! }
         std::optional< Reply> receive() noexcept;

         //! @return the file descriptor assocciated with this _call context_
         inline auto descriptor() const noexcept { return m_descriptor;}
             
      private:
         common::strong::ipc::descriptor::id m_descriptor;
         common::unique_function< std::optional< Reply>()> m_implementation;
         
      };

      namespace buffer
      {
         auto type( const std::vector< header::Field>& headers) -> std::string_view;
      } // buffer

   } // http::inbound::call
} // casual