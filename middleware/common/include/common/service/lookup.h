//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/uuid.h"
#include "common/message/service.h"

#include <string>

namespace casual
{
   namespace common
   {
      namespace service
      {
         struct Lookup
         {
            using Context = message::service::lookup::Request::Context;
            using Reply = message::service::lookup::Reply;
            using State = Reply::State;

            //!
            //! Lookup an entry point for the @p service
            //!
            Lookup( std::string service);

            //!
            //! Lookup an entry point for the @p service
            //! using a specific context
            //!  * regular
            //!  * no_reply
            //!  * forward
            //!  * gateway
            //!
            Lookup( std::string service, Context context);

            ~Lookup();

            //!
            //! @return the reply from the `broker`
            //!    can only be either idle (reserved) or busy.
            //!
            //!    if busy, a second invocation will block until it's idle
            //!
            //! @throws common::exception::xatmi::service::no::Entry if the service is not present or discovered
            //!
            Reply operator () () const;
         private:
            std::string m_service;
            mutable Uuid m_correlation;

         };

      } // service

   } // common

} // casual


