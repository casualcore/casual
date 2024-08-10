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
      namespace local
      {
         namespace
         {
            void discard( const strong::correlation::id& correlation) noexcept
            {
               if( ! correlation)
                  return;

               exception::guard( [ &]()
               {
                  Trace trace{ "common::service::lookup::discard"};

                  const auto request = [&]()
                  {
                     message::service::lookup::discard::Request result{ process::handle()};
                     result.correlation = correlation;
                     return result;
                  }();

                  auto reply = communication::ipc::call( communication::instance::outbound::service::manager::device(), request);

                  if( reply.state == decltype( reply.state)::replied)
                     communication::ipc::inbound::device().discard( correlation);
               });
            }

            void validate( const lookup::Reply& reply, const std::string& service)
            {
               using Enum = decltype( reply.state);
               switch( reply.state)
               {
                  case Enum::idle:
                     return;
                  case Enum::absent:
                     code::raise::error( code::xatmi::no_entry, "failed to lookup service: ", service);
                  case Enum::timeout:
                     code::raise::error( code::xatmi::timeout, "timeout during lookup of service: ", service);
               };
            }
            
         } // <unnamed>
      } // local

      Lookup::Lookup( std::string service, lookup::Context context, std::optional< platform::time::point::type> deadline)
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
         : Lookup( std::move( service), lookup::Context{}, std::move( deadline)) 
      {}

      Lookup::~Lookup()
      {
         // we have to discard if we're still pending with the service manager
         if( ! m_correlation)
            local::discard( m_correlation);
      }

      Lookup::Lookup( Lookup&& other) noexcept
         : m_service{ std::exchange( other.m_service, {})},
            m_correlation{ std::exchange( other.m_correlation, {})}
      {}

      Lookup& Lookup::operator = ( Lookup&& other) noexcept
      {
         m_service = std::exchange( other.m_service, {});
         m_correlation = std::exchange( other.m_correlation, {});
         return *this;
      }

      namespace lookup
      {
         lookup::Reply reply( Lookup&& lookup)
         {
            auto reply = communication::ipc::receive< lookup::Reply>( std::exchange( lookup.m_correlation, {}));

            local::validate( reply, lookup.m_service);
            return reply;
         }

         namespace non::blocking
         {
            std::optional< lookup::Reply> reply( Lookup& lookup)
            {
               if( auto reply = communication::ipc::non::blocking::receive< lookup::Reply>( lookup.m_correlation))
               {
                  lookup.m_correlation = {};
                  local::validate( *reply, lookup.m_service);
                  return reply;
               }
               return std::nullopt;
            }
         } // non::blocking
  
      } // lookups

   } // common::service
} // casual
