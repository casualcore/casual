//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "service/call/context.h"

#include "common/buffer/type.h"

namespace casual
{
   namespace service
   {
      namespace call
      {
         using Flag = casual::service::call::sync::Flag;
         using Result = casual::service::call::sync::Result;

         Result invoke( std::string service, common::buffer::payload::Send payload, Flag flags = Flag::no_flags);
         Result invoke( std::string service, const common::buffer::Payload& payload, Flag flags = Flag::no_flags);
      } // call

      namespace send
      {
         using Flag = casual::service::call::async::Flag;

         common::strong::correlation::id invoke( std::string service, common::buffer::payload::Send payload, Flag flags = Flag::no_flags);
         common::strong::correlation::id invoke( std::string service, const common::buffer::Payload& payload, Flag flags = Flag::no_flags);

      } // send

      namespace receive
      {
         using Result = casual::service::call::reply::Result;
         using Flag = casual::service::call::reply::Flag;

         Result invoke( const common::strong::correlation::id& correlation, Flag flags = Flag::no_flags);

         //! receives the next reply regardless of correlation. 
         //! `Flag::any` is implicit
         Result invoke( Flag flags = Flag{});
      } // receive

   } // service
} // casual


