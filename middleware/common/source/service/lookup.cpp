//!
//! casual
//!

#include "common/service/lookup.h"

#include "common/communication/ipc.h"

namespace casual
{
   namespace common
   {
      namespace service
      {
         Lookup::Lookup( std::string service, message::service::lookup::Request::Context context) : m_service( std::move( service))
         {
            message::service::lookup::Request request;
            request.requested = m_service;
            request.process = process::handle();
            request.context = context;

            m_correlation = communication::ipc::blocking::send( communication::ipc::broker::device(), request);
         }

         Lookup::Lookup( std::string service) : Lookup( std::move( service), message::service::lookup::Request::Context::regular) {}

         Lookup::~Lookup()
         {
            if( ! m_service.empty())
            {
               //
               //
               //
            }

            if( m_correlation != uuid::empty())
            {
               communication::ipc::inbound::device().discard( m_correlation);
            }
         }

         message::service::lookup::Reply Lookup::operator () () const
         {
            assert(  m_correlation != uuid::empty());

            message::service::lookup::Reply result;
            communication::ipc::blocking::receive( communication::ipc::inbound::device(), result, m_correlation);

            if( result.state != message::service::lookup::Reply::State::busy)
            {
               //
               // We're not expecting another message from broker
               //
               m_correlation = Uuid{};
            }
            return result;
         }

      } // service

   } // common

} // casual
