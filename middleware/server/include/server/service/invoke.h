//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/buffer/type.h"
#include "common/flag.h"

#include "common/strong/id.h"
#include "common/execution/context.h"

#include "common/flag/xatmi.h"

namespace casual
{
   namespace server::service::invoke
   {
      struct Service
      {
         std::string name;

         CASUAL_LOG_SERIALIZE({
            CASUAL_SERIALIZE( name);
         })
      };

      struct Parameter
      {
         //! these flags represent the possible flags that a service can be
         //! invoked with (tpservice template).
         enum class Flag : long
         {
            conversation = std::to_underlying( common::flag::xatmi::Flag::conversation),
            in_transaction = std::to_underlying( common::flag::xatmi::Flag::in_transaction),
            no_reply = std::to_underlying( common::flag::xatmi::Flag::no_reply),
            send_only = std::to_underlying( common::flag::xatmi::Flag::send_only),
            receive_only = std::to_underlying( common::flag::xatmi::Flag::receive_only),
         };

         // indicate that this enum is used as a flag, and uses xatmi flags as a superset
         friend consteval common::flag::xatmi::Flag casual_enum_as_flag_superset( Flag);

         Flag flags{};
         Service service;
         common::execution::context::Parent parent;
         common::buffer::Payload payload;
         common::strong::conversation::descriptor::id descriptor;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( flags);
            CASUAL_SERIALIZE( service);
            CASUAL_SERIALIZE( parent);
            CASUAL_SERIALIZE( payload);
            CASUAL_SERIALIZE( descriptor);
         )
      };

      static_assert( concepts::movable< Parameter>);

      namespace result
      {
         struct Code 
         {
            common::flag::xatmi::Return result = common::flag::xatmi::Return::success;
            long user{};
   
            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( result);
               CASUAL_SERIALIZE( user);
            )
         };
         
      } // result

      struct Result
      {
         common::buffer::Payload payload;
         result::Code code;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( payload);
            CASUAL_SERIALIZE( code);
         )
      };

      static_assert( concepts::movable< Result>);

      struct Forward
      {
         Parameter parameter;
      };

   } // server::service::invoke
} // casual


