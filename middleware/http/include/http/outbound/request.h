//!
//! casual
//!

#ifndef MIDDLEWARE_HTTP_INCLUDE_HTTP_OUTBOUND_REQUEST_H_
#define MIDDLEWARE_HTTP_INCLUDE_HTTP_OUTBOUND_REQUEST_H_

#include "common/platform.h"

namespace casual
{
   namespace http
   {
      namespace request
      {
         struct Reply
         {
            std::vector< std::string> header;
            common::platform::binary::type payload;
         };

         Reply post( const std::string& url, const common::platform::binary::type& payload, const std::vector< std::string>& header);


      } // request
   } // http
} // casual


#endif /* MIDDLEWARE_HTTP_INCLUDE_HTTP_OUTBOUND_REQUEST_H_ */
