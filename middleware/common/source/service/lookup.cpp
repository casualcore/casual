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
         Lookup::Lookup( std::string service, Context context) : m_service( std::move( service))
         {
            message::service::lookup::Request request;
            request.requested = m_service;
            request.process = process::handle();
            request.context = context;

            m_correlation = communication::ipc::blocking::send( communication::ipc::service::manager::device(), request);
         }

         Lookup::Lookup( std::string service) : Lookup( std::move( service), Context::regular) {}

         Lookup::~Lookup()
         {
            if( m_correlation != uuid::empty())
            {
               communication::ipc::inbound::device().discard( m_correlation);
            }
         }

         Lookup::Reply Lookup::operator () () const
         {
            assert(  m_correlation != uuid::empty());

            Reply result;
            communication::ipc::blocking::receive( communication::ipc::inbound::device(), result, m_correlation);

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
