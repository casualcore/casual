//!
//! casual
//!

#ifndef MIDDLEWARE_HTTP_INCLUDE_HTTP_OUTBOUND_REQUEST_H_
#define MIDDLEWARE_HTTP_INCLUDE_HTTP_OUTBOUND_REQUEST_H_

#include "common/platform.h"
#include "common/buffer/type.h"

namespace casual
{
   namespace http
   {
      namespace request
      {
         namespace payload
         {
            using Request = common::buffer::payload::Send;
            using Reply = common::buffer::Payload;

         } // payload

         struct Reply
         {
            std::vector< std::string> header;
            payload::Reply payload;
         };

         Reply post( const std::string& url, const payload::Request& payload);
         Reply post( const std::string& url, const payload::Request& payload, const std::vector< std::string>& header);


      } // request
   } // http
} // casual


#endif /* MIDDLEWARE_HTTP_INCLUDE_HTTP_OUTBOUND_REQUEST_H_ */
