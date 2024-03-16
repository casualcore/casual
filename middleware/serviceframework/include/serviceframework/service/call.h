//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once




#include "common/buffer/type.h"
#include "common/service/call/context.h"



namespace casual
{
   namespace serviceframework::service
   {
      using payload_type = common::buffer::Payload;
      using descriptor_type = platform::descriptor::type;


      namespace call
      {
         using Flag = common::service::call::sync::Flag;
         using Result = common::service::call::sync::Result;

         Result invoke( std::string service, const payload_type& payload, Flag flags = Flag{});
      } // call

      namespace send
      {
         using Flag = common::service::call::async::Flag;

         descriptor_type invoke( std::string service, const payload_type& payload, Flag flags = Flag{});

      } // send

      namespace receive
      {
         using Result = common::service::call::reply::Result;
         using Flag = common::service::call::reply::Flag;

         Result invoke( descriptor_type descriptor, Flag flags = Flag{});
      } // receive

   } // serviceframework::service
} // casual


