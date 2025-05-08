//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/buffer/type.h"
#include "common/flag.h"
#include "common/service/header.h"
#include "common/strong/id.h"
#include "common/execution/context.h"

#include "common/flag/xatmi.h"

namespace casual
{
   namespace common
   {
      namespace service
      {
         namespace invoke
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
                  conversation = std::to_underlying( flag::xatmi::Flag::conversation),
                  in_transaction = std::to_underlying( flag::xatmi::Flag::in_transaction),
                  no_reply = std::to_underlying( flag::xatmi::Flag::no_reply),
                  send_only = std::to_underlying( flag::xatmi::Flag::send_only),
                  receive_only = std::to_underlying( flag::xatmi::Flag::receive_only),
               };

               // indicate that this enum is used as a flag, and uses xatmi flags as a superset
               friend consteval flag::xatmi::Flag casual_enum_as_flag_superset( Flag);

               Flag flags{};
               Service service;
               common::service::header::Fields header;
               common::execution::context::Parent parent;
               buffer::Payload payload;
               strong::conversation::descriptor::id descriptor;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( flags);
                  CASUAL_SERIALIZE( service);
                  CASUAL_SERIALIZE( header);
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
                  flag::xatmi::Return result = flag::xatmi::Return::success;
                  long user{};
         
                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( result);
                     CASUAL_SERIALIZE( user);
                  )
               };
               
            } // result

            struct Result
            {
               buffer::Payload payload;
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

         } // invoke
      } // service
   } // common
} // casual


