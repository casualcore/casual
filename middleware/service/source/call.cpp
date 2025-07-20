//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "service/call.h"
#include "service/call/context.h"

#include "common/log.h"


namespace casual
{
   namespace service
   {
      namespace call
      {
         Result invoke( std::string service, const payload_type& payload, const Complement& complement)
         {
            common::Trace trace{ "service::call::invoke"};

            return call::context().sync( std::move( service), payload, complement.flags, complement.header);
         }
      } // call

      namespace send
      {
         descriptor_type invoke( std::string service, const payload_type& payload, const Complement& complement)
         {
            common::Trace trace{ "service::send::invoke"};

            return call::context().async( std::move( service), payload, complement.flags, complement.header);
         }

      } // send

      namespace receive
      {
         Result invoke( descriptor_type descriptor, Flag flags)
         {
            common::Trace trace{ "service::receive::invoke"};

            return call::context().reply( descriptor, flags);
         }
      } // receive

   } // service
} // casual
