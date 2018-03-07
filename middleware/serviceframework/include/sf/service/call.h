//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_SERVICEFRAMEWORK_INCLUDE_SF_SERVICE_CALL_H_
#define CASUAL_MIDDLEWARE_SERVICEFRAMEWORK_INCLUDE_SF_SERVICE_CALL_H_



#include "common/buffer/type.h"
#include "common/service/call/context.h"



namespace casual
{
   namespace sf
   {
      namespace service
      {
         using payload_type = common::buffer::Payload;
         using descriptor_type = common::platform::descriptor::type;


         namespace call
         {
            using Flag = common::service::call::sync::Flag;
            using Flags = common::service::call::sync::Flags;
            using Result = common::service::call::sync::Result;

            Result invoke( const std::string& service, const payload_type& paylaod, Flags flags = Flags{});
         } // call

         namespace send
         {
            using Flag = common::service::call::async::Flag;
            using Flags = common::service::call::async::Flags;

            descriptor_type invoke( const std::string& service, const payload_type& paylaod, Flags flags = Flags{});

         } // send

         namespace receive
         {
            using Result = common::service::call::reply::Result;
            using Flag = common::service::call::reply::Flag;
            using Flags = common::service::call::reply::Flags;

            Result invoke( descriptor_type descriptor, Flags flags = Flags{});
         } // receive

      } // service
   } // sf
} // casual

#endif // CASUAL_MIDDLEWARE_SERVICEFRAMEWORK_INCLUDE_SF_SERVICE_CALL_H_
