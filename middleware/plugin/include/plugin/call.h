//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <optional>

namespace casual
{
   namespace plugin::call
   {  
      struct Payload
      {
         std::vector< std::string> header;
         std::vector< char> body;
      };

      struct Arguments
      {
         std::string service;
         Payload payload;
      };

      struct Reply
      {
         Payload payload;
      };

      //! the call context that holds the state machine for a service call
      //! 
      //! possible initialization: `nginx_context.user_data = new plugin::call::Context{ std::move( arguments)};
      struct Context
      {
         Context( Arguments arguments);
         ~Context();

         //! @returns the reply if ready, absent if not.
         //! @note if a reply is returned then this context is done, and the only valid operation
         //!   is calling the destructor.
         //! TODO: on error should `Reply` contain the error_code, or should we throw std::system_error with error_code and description? 
         //!    I think the latter...
         std::optional< Reply> receive();
      
      private:
         // TODO: state... Should we use pimpl?... Don't think so, we would get two indirections for every interaction instad of one.

      };



   } // plugin::call
} // casual