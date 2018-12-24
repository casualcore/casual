//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/service/lookup.h"

#include "common/communication/instance.h"
#include "common/exception/xatmi.h"
#include "common/exception/handle.h"


namespace casual
{
   namespace common
   {
      namespace service
      {
         namespace detail
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
                  try
                  {
                     log::line( log::debug, "pending lookup - discard");

                     // we've got a pending lookup on the service, we need to tell
                     // service manager to discard our lookup.

                     const auto request = [&]()
                     {
                        message::service::lookup::discard::Request result;
                        result.requested = m_service;
                        result.process = process::handle();
                        result.correlation = m_correlation;
                        return result;
                     }();

                     auto reply = communication::ipc::call( communication::instance::outbound::service::manager::device(), request);

                     if( reply.state == decltype( reply.state)::replied)
                        communication::ipc::inbound::device().discard( m_correlation);
                  }
                  catch( ...)
                  {
                     exception::handle();
                  }
               }
            }

            Lookup::Lookup( Lookup&&) noexcept = default;
            Lookup& Lookup::operator = ( Lookup&&) noexcept = default;

            

            std::ostream& operator << ( std::ostream& out, const Lookup& value)
            {
               return out << "{ service: " << value.m_service
                  << ", correlation: " << value.m_correlation
                  << '}';
            }

            bool Lookup::update( Reply&& reply) const
            {
               log::line( verbose::log, "reply: ", reply);

               switch( reply.state)
               {
                  case State::idle:
                     m_correlation = Uuid{};
                     m_reply = std::move( reply);
                     return true;
                     break;
                  case State::absent:
                     m_correlation = Uuid{};
                     throw common::exception::xatmi::service::no::Entry{ m_service};
                     break;
                  case State::busy:
                     m_reply = std::move( reply);
                     break;
               }
               return false;
            }
         } // detail

         const Lookup::Reply& Lookup::operator () () const
         {
            Trace trace{ "common::service::Lookup::operator()"};

            if( ! m_reply || ! uuid::empty( m_correlation))
            {
               Reply result;
               communication::ipc::blocking::receive( communication::ipc::inbound::device(), result, m_correlation);
               update( std::move( result));
            }

            return m_reply.value();
         }

         namespace non
         {
            namespace blocking
            {
               Lookup::operator bool () const
               {
                  if( ! m_reply || ! uuid::empty( m_correlation))
                  {
                     detail::Lookup::Reply result;
                     if( communication::ipc::non::blocking::receive( communication::ipc::inbound::device(), result, m_correlation))
                     {
                        update( std::move( result));
                     }
                  }
                  return m_reply.has_value() && ! m_reply.value().busy();
               }
   /*
               Lookup::operator service::Lookup () &&
               {  
                  if( ! m_reply)
                  {
                     Reply result;
                     communication::ipc::blocking::receive( communication::ipc::inbound::device(), result, m_correlation);

                     if( ! update( result))
                     {
                        communication::ipc::blocking::receive( communication::ipc::inbound::device(), result, m_correlation);
                     }
                     m_reply = std::move( result);
                  }
                  return m_reply.value();
               }
               */

            } // blocking
         } // non
      } // service
   } // common
} // casual
