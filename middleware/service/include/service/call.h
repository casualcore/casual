//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "service/call/context.h"

#include "common/buffer/type.h"

#include "casual/header.h"


namespace casual
{
   namespace service
   {
      using payload_type = common::buffer::Payload;
      using descriptor_type = platform::descriptor::type;


      namespace call
      {
         using Flag = casual::service::call::sync::Flag;
         using Result = casual::service::call::sync::Result;

         struct Complement
         {
            Flag flags = Flag::no_flags;
            header::Fields header;
         };

         Result invoke( std::string service, const payload_type& payload, const Complement& complement = {});
      } // call

      namespace send
      {
         using Flag = casual::service::call::async::Flag;

         struct Complement
         {
            Flag flags = Flag::no_flags;
            header::Fields header;
         };

         descriptor_type invoke( std::string service, const payload_type& payload, const Complement& complement = {});

      } // send

      namespace receive
      {
         using Result = casual::service::call::reply::Result;
         using Flag = casual::service::call::reply::Flag;

         Result invoke( descriptor_type descriptor, Flag flags = Flag{});
      } // receive

   } // service
} // casual


