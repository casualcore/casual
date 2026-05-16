//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "service/call/state.h"
#include "service/lookup.h"

#include "casual/header.h"

#include "common/message/service.h"
#include "common/flag/service/call.h"


#include <vector>


namespace casual
{
   namespace service::call
   {
      namespace async = common::flag::service::call::async;

      namespace reply
      {
         using namespace common::flag::service::call::reply;
         struct Result
         {
            common::buffer::Payload buffer;
            long user = 0;
            common::strong::correlation::id correlation;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( buffer);
               CASUAL_SERIALIZE( user);
               CASUAL_SERIALIZE( correlation);
            )

         };
      } // reply

      namespace sync
      {
         using namespace common::flag::service::call::sync;
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

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( result);
         )
      };

      struct Context
      {
         static Context& instance();

         common::strong::correlation::id async( const std::string& service, common::buffer::payload::Send buffer, async::Flag flags);
         common::strong::correlation::id async( service::Lookup&& lookup, common::buffer::payload::Send buffer, async::Flag flags);

         reply::Result reply( const common::strong::correlation::id& correlation, reply::Flag flags);
         //! receives the next reply regardless of correlation.
         //! `reply::Flag::any` is implicit
         reply::Result reply( reply::Flag flags);

         sync::Result sync( const std::string& service, common::buffer::payload::Send buffer, sync::Flag flags);

         void cancel( const common::strong::correlation::id& correlation);

         void clear();

         //! @returns true if there are pending replies or associated transactions.
         //!  Hence, it's ok to do a service-forward if false is return
         bool pending() const;

         //! set deadline for calls.
         void deadline( common::chronology::time_point now, std::optional< common::chronology::duration> timeout);
         std::optional< common::chronology::time_point> deadline() const;

         //! Tries to finalize the context, wait for "all" pending replies that is found in `transaction_associated`.
         void finalize( std::span< const common::strong::correlation::id> transaction_associated);

         bool empty() const;

      private:
         Context();
         bool receive( common::message::service::call::Reply& reply, const common::strong::correlation::id& correlation, reply::Flag);

         State m_state;
      };

      inline Context& context() { return Context::instance();}

   } // service::call
} // casual



