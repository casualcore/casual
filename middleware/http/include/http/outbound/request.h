//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/platform.h"
#include "common/buffer/type.h"
#include "common/service/header.h"

namespace casual
{
   namespace http
   {
      using Header = common::service::header::Fields;

      namespace request
      {
         namespace payload
         {
            using Request = common::buffer::payload::Send;
            using Reply = common::buffer::Payload;

         } // payload

         struct Reply
         {
            Header header;
            payload::Reply payload;
         };

         Reply post( const std::string& url, const payload::Request& payload);
         Reply post( const std::string& url, const payload::Request& payload, const Header& header);


      } // request
   } // http
} // casual



