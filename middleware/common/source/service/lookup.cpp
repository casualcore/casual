//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/service/lookup.h"

#include "common/communication/instance.h"


namespace casual
{
   namespace common::service
   {
      namespace detail
      {
         Lookup::Lookup() noexcept {};

         Lookup::Lookup( std::string service, Context context, std::optional< platform::time::point::type> deadline)
            : m_service( std::move( service))
         {
            Trace trace{ "common::service::Lookup"};

            message::service::lookup::Request request{ process::handle()};
            request.requested = m_service;
            request.context = context;
            request.deadline = std::move( deadline);

            m_correlation = communication::device::blocking::send( communication::instance::outbound::service::manager::device(), request);
         }

         Lookup::Lookup( std::string service, std::optional< platform::time::point::type> deadline) 
            : Lookup( std::move( service), Context::regular, std::move( deadline)) 
         {}

         Lookup::~Lookup()
         {
            if( m_correlation)
            {
               try
               {
                  log::line( log::debug, "pending lookup - discard");

                  // we've got a pending lookup on the service, we need to tell
                  // service manager to discard our lookup.

                  const auto request = [&]()
                  {
                     message::service::lookup::discard::Request result{ process::handle()};
                     result.requested = m_service;
                     result.correlation = m_correlation;
                     return result;
                  }();

                  auto reply = communication::ipc::call( communication::instance::outbound::service::manager::device(), request);

                  if( reply.state == decltype( reply.state)::replied)
                     communication::ipc::inbound::device().discard( m_correlation);
               }
               catch( ...)
               {
                  log::line( log::category::error, exception::capture());
               }
            }
         }

         Lookup::Lookup( Lookup&& other) noexcept
            : m_service{ std::exchange( other.m_service, {})},
               m_correlation{ std::exchange( other.m_correlation, {})},
               m_reply{ std::exchange( other.m_reply, {})}
         {}


         Lookup& Lookup::operator = ( Lookup&& other) noexcept
         {
            m_service = std::exchange( other.m_service, {});
            m_correlation = std::exchange( other.m_correlation, {});
            m_reply = std::exchange( other.m_reply, {});
            return *this;
         }

         void swap( Lookup& lhs, Lookup& rhs)
         {
            using std::swap;
            swap( lhs.m_service, rhs.m_service);
            swap( lhs.m_reply, rhs.m_reply);
            swap( lhs.m_correlation, rhs.m_correlation);
         }


         bool Lookup::update( Reply&& reply)
         {
            log::line( verbose::log, "reply: ", reply);

            switch( reply.state)
            {
               case State::idle:
                  m_correlation = {};
                  m_reply = std::move( reply);
                  return true;
                  break;
               case State::absent:
                  m_correlation = {};
                  code::raise::error( code::xatmi::no_entry, "lookup: ", *this);
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

         if( ! m_reply || m_correlation)
         {
            Reply result;
            communication::device::blocking::receive( communication::ipc::inbound::device(), result, m_correlation);
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
               if( ! m_reply || m_correlation)
               {
                  detail::Lookup::Reply result;
                  if( communication::device::non::blocking::receive( communication::ipc::inbound::device(), result, m_correlation))
                     update( std::move( result));
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
   } // common::service
} // casual
