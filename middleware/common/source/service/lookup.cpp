//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/service/lookup.h"

#include "common/communication/instance.h"
#include "common/exception/xatmi.h"


namespace casual
{
   namespace common
   {
      namespace service
      {
         Lookup::Lookup( std::string service, Context context) : m_service( std::move( service))
         {
            Trace trace{ "common::service::Lookup"};

            message::service::lookup::Request request;
            request.requested = m_service;
            request.process = process::handle();
            request.context = context;

            m_correlation = communication::ipc::blocking::send( communication::instance::outbound::service::manager::device(), request);
         }

         Lookup::Lookup( std::string service) : Lookup( std::move( service), Context::regular) {}

         Lookup::~Lookup()
         {
            if( m_correlation != uuid::empty())
            {
               log::line( log::debug, "pending lookup - discard");

               // we've got a pending lookup on the service, we need to tell
               // service manager to discard our lookup.
               message::service::lookup::discard::Request request;
               {
                  request.requested = m_service;
                  request.process = process::handle();
                  request.correlation = m_correlation;
               }
               auto reply = communication::ipc::call( communication::instance::outbound::service::manager::device(), request);

               if( reply.state == decltype( reply.state)::replied)
                  communication::ipc::inbound::device().discard( m_correlation);
            }
         }

         Lookup::Reply Lookup::operator () () const
         {
            Trace trace{ "common::service::Lookup::operator()"};

            assert( ! uuid::empty( m_correlation));

            Reply result;
            communication::ipc::blocking::receive( communication::ipc::inbound::device(), result, m_correlation);

           log::line( verbose::log, "reply: ", result);

            switch( result.state)
            {
               case Reply::State::idle:
                  m_correlation = Uuid{};
                  break;
               case Reply::State::absent:
                  m_correlation = Uuid{};
                  throw common::exception::xatmi::service::no::Entry{ m_service};
                  break;
               case Reply::State::busy:
                  // no-op
                  break;
            }
            return result;
         }

      } // service

   } // common

} // casual
