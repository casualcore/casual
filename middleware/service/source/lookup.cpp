//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "service/lookup.h"
#include "service/common.h"

#include "common/communication/instance.h"

namespace casual
{
   namespace service
   {
      namespace lookup
      {
         namespace local
         {
            namespace
            {
               void discard( const common::strong::correlation::id& correlation) noexcept
               {
                  if( ! correlation)
                     return;

                  common::exception::guard( [ &]()
                  {
                     Trace trace{ "common::service::lookup::local::discard"};

                     const auto request = [&]()
                     {
                        common::message::service::lookup::discard::Request result{ common::process::handle()};
                        result.correlation = correlation;
                        return result;
                     }();

                     auto reply = common::communication::ipc::call( common::communication::instance::outbound::service::manager::device(), request);

                     if( reply.state == decltype( reply.state)::replied)
                        common::communication::ipc::inbound::device().discard( correlation);
                  });
               }

               void validate( const lookup::Reply& reply, const std::string& service)
               {
                  Trace trace{ "common::service::lookup::local::validate"};
                  common::log::debug( "reply: ", reply);

                  using Enum = decltype( reply.state);
                  switch( reply.state)
                  {
                     case Enum::idle:
                        return;
                     case Enum::absent:
                        common::code::raise::error( common::code::xatmi::no_entry, "failed to lookup service: ", service);
                     case Enum::timeout:
                        common::code::raise::error( common::code::xatmi::timeout, "timeout during lookup of service: ", service);
                  };
               }
               
            } // <unnamed>
         } // local
         
         lookup::Reply reply( Lookup&& lookup)
         {
            if( ! lookup.m_correlation)
               common::code::raise::error( common::code::casual::invalid_argument, "lookup is already consumed: ", lookup);

            auto reply = common::communication::ipc::receive< lookup::Reply>( std::exchange( lookup.m_correlation, {}));

            local::validate( reply, lookup.m_service);
            return reply;
         }

         namespace non::blocking
         {
            std::optional< lookup::Reply> reply( Lookup& lookup)
            {
               if( ! lookup.m_correlation)
                  common::code::raise::error( common::code::casual::invalid_argument, "lookup is already consumed: ", lookup);

               if( auto reply = common::communication::ipc::non::blocking::receive< lookup::Reply>( lookup.m_correlation))
               {
                  lookup.m_correlation = {};
                  local::validate( *reply, lookup.m_service);
                  return reply;
               }
               return std::nullopt;
            }
         } // non::blocking
  
      } // lookup

      Lookup::Lookup( std::string service, const common::transaction::ID& trid, lookup::Context context, std::optional< platform::time::point::type> deadline)
         : m_service( std::move( service))
      {
         Trace trace{ "common::service::Lookup"};

         common::message::service::lookup::Request request{ common::process::handle()};
         request.requested = m_service;
         request.context = context;
         request.trid = trid;
         request.deadline = std::move( deadline);

         m_correlation = common::communication::device::blocking::send( common::communication::instance::outbound::service::manager::device(), request);
      }

      Lookup::Lookup( std::string service, const common::transaction::ID& trid, std::optional< platform::time::point::type> deadline) 
         : Lookup( std::move( service), trid, lookup::Context{}, std::move( deadline)) 
      {}

      Lookup::~Lookup()
      {
         // we have to discard if we're still pending with the service manager
         if( m_correlation)
            lookup::local::discard( m_correlation);
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

   } // service
} // casual
