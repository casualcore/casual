//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/manager/service/invoke.h"

#include "common/serialize/service/protocol.h"

namespace casual
{
   namespace manager::service::protocol
   {
      common::serialize::service::Protocol deduce( invoke::Parameter&& parameter);

      //! a wrapper for common::serialize::service::user to be used in managers
      //! and use the Parameter and Result types
      template< typename... Ts>
      invoke::Result dispatch( invoke::Parameter&& parameter, Ts&&... ts)
      {
         invoke::Result result;
         result.payload = common::serialize::service::user( 
            std::move( parameter.payload), parameter.header, 
            std::forward< Ts>( ts)...);

         return result;
      }

      template< typename F, typename... Ts>
      invoke::Result dispatch( common::serialize::service::Protocol&& protocol, F&& function, Ts&&... ts)
      {
         invoke::Result result;
         result.payload = common::serialize::service::user( 
            std::move( protocol), std::forward< F>( function), 
            std::forward< Ts>( ts)...);

         return result;
      }

      
   } // manager::service::protocol
   
} // casual