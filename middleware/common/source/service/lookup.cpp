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
            Lookup::Lookup() noexcept {};

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

// casual compiler compatible
# if defined __GNUC__
#  if (__GNUC__ == 7) && (__GNUC_MINOR__ == 3)

            Lookup& Lookup::operator = ( Lookup&& other) noexcept
            {
               m_service = std::move( other.m_service);
               m_correlation = std::move( other.m_correlation);
               m_reply = std::move( other.m_reply);
               return *this;
            }
#  else
            Lookup& Lookup::operator = ( Lookup&&) noexcept = default;
#  endif
#endif

            void swap( Lookup& lhs, Lookup& rhs)
            {
               std::swap( lhs.m_service, rhs.m_service);
               std::swap( lhs.m_reply, rhs.m_reply);
               std::swap( lhs.m_correlation, rhs.m_correlation);
            }

            std::ostream& operator << ( std::ostream& out, const Lookup& value)
            {
               return out << "{ service: " << value.m_service
                  << ", correlation: " << value.m_correlation
                  << '}';
            }

            bool Lookup::update( Reply&& reply)
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

         const Lookup::Reply& Lookup::operator () ()
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
               Lookup::operator bool ()
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

               Lookup::operator service::Lookup () &&
               {
                  service::Lookup result;
                  swap( result, *this);
                  return result;
               }
            } // blocking
         } // non
      } // service
   } // common
} // casual
