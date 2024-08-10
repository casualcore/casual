//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/service/call/state.h"
#include "common/service/lookup.h"

#include "common/flag/service/call.h"

#include "common/message/service.h"


#include <vector>


namespace casual
{
   namespace common::service::call
   {
      namespace async = flag::service::call::async;

      namespace reply
      {
         using namespace flag::service::call::reply;
         struct Result
         {
            common::buffer::Payload buffer;
            long user = 0;
            descriptor_type descriptor;
         };
      } // reply

      namespace sync
      {
         using namespace flag::service::call::sync;
         struct Result
         {
            common::buffer::Payload buffer;
            long user = 0;
         };
      } // sync

      //! Will be thrown if service fails (application error)
      struct Fail
      {
         reply::Result result;
         inline friend std::ostream& operator << ( std::ostream& out, const Fail& value) 
         { 
            return out << "{ buffer: " << value.result.buffer
               << ", buffer: " << value.result.descriptor
               << ", user: " << value.result.user
               << '}';
         }
      };

      struct Context
      {
         static Context& instance();

         descriptor_type async( const std::string& service, common::buffer::payload::Send buffer, async::Flag flags);

         descriptor_type async( service::Lookup&& lookup, common::buffer::payload::Send buffer, async::Flag flags);

         descriptor_type async( service::Lookup&& lookup, common::buffer::payload::Send buffer, service::header::Fields header, async::Flag flags);

         reply::Result reply( descriptor_type descriptor, reply::Flag flags);

         sync::Result sync( const std::string& service, common::buffer::payload::Send buffer, sync::Flag flags);

         void cancel( descriptor_type descriptor);

         void clean();

         //! @returns true if there are pending replies or associated transactions.
         //!  Hence, it's ok to do a service-forward if false is return
         bool pending() const;

         //! set deadline for calls.
         void deadline( platform::time::point::type now, platform::time::unit timeout);
         std::optional< platform::time::point::type> deadline() const;

      private:
         Context();
         bool receive( message::service::call::Reply& reply, descriptor_type descriptor, reply::Flag);

         State m_state;
      };

      inline Context& context() { return Context::instance();}

   } // common::service::call
} // casual



