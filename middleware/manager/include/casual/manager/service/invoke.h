//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/flag/xatmi.h"
#include "casual/header.h"
#include "common/buffer/type.h"

#include <string>


namespace casual
{
   namespace manager::service::invoke
   {
      struct Parameter
      {
         //! these flags represent the possible flags that a service can be
         //! invoked with (tpservice template).
         enum class Flag : long
         {
            no_reply = std::to_underlying( common::flag::xatmi::Flag::no_reply),
         };

         // indicate that this enum is used as a flag, and uses xatmi flags as a superset
         friend consteval common::flag::xatmi::Flag casual_enum_as_flag_superset( Flag);

         Flag flags{};
         std::string service;
         header::Fields header;
         common::buffer::Payload payload;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( flags);
            CASUAL_SERIALIZE( service);
            CASUAL_SERIALIZE( header);
            CASUAL_SERIALIZE( payload);
         )
      };



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

      namespace concurrent
      {
         using callback_function_type = std::function< void( Result&&)>;

         struct Parameter
         {
            invoke::Parameter invoke;
            callback_function_type callback;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( invoke);
               CASUAL_SERIALIZE( callback);
            )
         };
         
      } // concurrent

      
   } // manager::service::invoke
} // casual