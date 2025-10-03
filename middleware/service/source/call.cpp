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
         Result invoke( std::string service, common::buffer::payload::Send payload, const Complement& complement)
         {
            common::Trace trace{ "service::call::invoke"};

            return call::context().sync( std::move( service), payload, complement.flags, complement.header);
         }

         Result invoke( std::string service, const common::buffer::Payload& payload, const Complement& complement)
         {
            return invoke( std::move( service), common::buffer::payload::Send{ payload}, complement);
         }

      } // call

      namespace send
      {
         common::strong::correlation::id invoke( std::string service, common::buffer::payload::Send payload, const Complement& complement)
         {
            common::Trace trace{ "service::send::invoke"};

            return call::context().async( std::move( service), payload, complement.flags, complement.header);
         }

         common::strong::correlation::id invoke( std::string service, const common::buffer::Payload& payload, const Complement& complement)
         {
            return invoke( std::move( service), common::buffer::payload::Send{ payload}, complement);
         }

      } // send

      namespace receive
      {
         Result invoke( const common::strong::correlation::id& correlation, Flag flags)
         {
            common::Trace trace{ "service::receive::invoke"};

            return call::context().reply( correlation, flags);
         }

         Result invoke( Flag flags)
         {
            common::Trace trace{ "service::receive::invoke"};

            return call::context().reply( flags);
         }

      } // receive

   } // service
} // casual
