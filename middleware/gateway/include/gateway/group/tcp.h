//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/group/tcp/logical/connect.h"
#include "gateway/common.h"
#include "gateway/message.h"

#include "common/communication/tcp.h"
#include "common/communication/ipc.h"
#include "common/communication/select.h"
#include "common/algorithm.h"
#include "common/signal/timer.h"
#include "common/environment.h"

#include "casual/assert.h"

#include <chrono>

namespace casual
{
   namespace gateway::group::tcp
   {

      template< typename S, typename LC, typename M>
      common::strong::correlation::id send( S& state, LC&& lost_callback, common::strong::socket::id descriptor, M&& message)
      {
         if( auto connection = state.connections.find_external( descriptor))
         {
            try
            {
               return connection->send( state.directive, std::forward< M>( message));
            }
            catch( ...)
            {
               const auto error = common::exception::capture();

               auto lost = lost_callback( state, descriptor);

               if( error.code() != common::code::casual::communication_unavailable)
                  common::log::line( common::log::category::error, error, " send failed to remote: ", lost.remote, " - action: remove connection");

               // we 'lost' the connection in some way - we put a connection::Lost on our own ipc-device, and handle it
               // later (and differently depending on if we're inbound, outbound, 'regular' or 'reversed')
               common::communication::ipc::inbound::device().push( std::move( lost));
            }
         }
         else
         {
            common::log::line( common::log::category::error, common::code::casual::internal_correlation, " tcp::send -  failed to correlate descriptor: ", descriptor, " for message type: ", message.type());
            common::log::line( common::log::category::verbose::error, "state: ", state);
         }

         return {};
      }

      //! Represent a tcp-connection, or rather a send/receive abstraction for the socket it own.
      struct Connection
      {
         using complete_type = common::communication::tcp::message::Complete;

         inline explicit Connection( common::communication::Socket&& socket, message::protocol::Version protocol)
            : m_device{ std::move( socket)}, m_protocol{ protocol} {}

      
         inline auto descriptor() const noexcept { return m_device.connector().descriptor();}

         //! tries to send as much of unsent as possible, if any.
         void unsent( common::communication::select::Directive& directive);
         
         template< typename M>
         auto send( common::communication::select::Directive& directive, M&& message)
         {
            return send( directive, common::serialize::native::complete< complete_type>( std::forward< M>( message)));
         }

         common::strong::correlation::id send( common::communication::select::Directive& directive, complete_type&& complete);

         inline auto next()
         {
            return common::communication::device::non::blocking::next( m_device);
         }

         inline auto protocol() const noexcept { return m_protocol;}
         inline void protocol( message::protocol::Version protocol) noexcept { m_protocol = protocol;}

         inline friend bool operator == ( const Connection& lhs, common::strong::file::descriptor::id rhs) { return lhs.descriptor() == rhs;}
         
         CASUAL_LOG_SERIALIZE( 
            CASUAL_SERIALIZE_NAME( m_device, "device");
            CASUAL_SERIALIZE_NAME( m_unsent, "unsent");
            CASUAL_SERIALIZE_NAME( m_protocol, "protocol");
         )

      private:
         std::vector< complete_type> m_unsent;
         common::communication::tcp::Duplex m_device;
         message::protocol::Version m_protocol;
      };

      namespace connection
      {
         template< typename Configuration>
         struct Information
         {
            Information( common::strong::socket::id descriptor,
               common::domain::Identity domain,
               Configuration configuration)
               : descriptor{ descriptor}, domain{ std::move( domain)}, configuration{ std::move( configuration)}
            {}

            common::strong::socket::id descriptor;
            common::domain::Identity domain;
            Configuration configuration;
            platform::time::point::type created = platform::time::clock::type::now();

            inline friend bool operator == ( const Information& lhs, common::strong::socket::id rhs) { return lhs.descriptor == rhs;} 
            inline friend bool operator == ( const Information& lhs, const common::strong::domain::id& rhs) { return lhs.domain == rhs;} 
            
            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( descriptor);
               CASUAL_SERIALIZE( domain);
               CASUAL_SERIALIZE( configuration);
               CASUAL_SERIALIZE( created);
            )
         };
         
      } // connection

      namespace detail::handle::communication
      {
         template< typename State, typename Lost>
         void exception( State& state, common::strong::socket::id descriptor, Lost lost) noexcept
         {
            try
            {
               throw;
            }
            catch( ...)
            {
               using namespace common;

               auto error = exception::capture();
               if( error.code() != code::casual::communication_unavailable)
               {
                  auto information = casual::assertion( state.connections.information( descriptor), "failed to find information for descriptor: ", descriptor);
                  log::line( log::category::error, "failed to communicate with domain: ", information->domain, ", configured address: ", information->configuration.address, " - error: ", error);
               }

               // we 'lost' the connection in some way - we put a connection::Lost on our own ipc-device, and handle it
               // later (and differently depending on if we're 'regular' or 'reversed')
               //
               // NOTE: lost functor might throw, and we have noexcept. If we fail to execute lost and handle the lost connection
               // properly there's no point in going on, we have a broken state in some way. We still catch and log the
               // error to help find potential future errors in lost (it should not throw).
               // TODO: Make sure lost is noexcept (compile time) and force the responsibility to "where it belongs"?
               try
               {
                  common::communication::ipc::inbound::device().push( lost( state, descriptor));
               }
               catch( ...)
               {
                  casual::terminate( "failed to handle lost connection for descriptor: ", descriptor, " - error: ", exception::capture());
               }
            }
         }
      } // detail::handle::communication

      namespace handle::dispatch
      {
         template< typename Policy, typename State, typename Handler, typename Lost>
         auto create( State& state, Handler handler, Lost lost)
         {
            return [ &state, handler = std::move( handler), lost = std::move( lost)]( common::strong::file::descriptor::id fd, common::communication::select::tag::read) mutable
            {
               // we know the descriptor is a socket descriptor, we convert.
               auto descriptor = common::strong::socket::id{ fd};

               if( auto connection = state.connections.find_external( descriptor))
               {
                  try
                  {
                     auto count = Policy::next::tcp();

                     
                     while( count-- > 0 && handler( connection->next(), descriptor))
                        ; // no-op
                  }
                  catch( ...)
                  {
                     detail::handle::communication::exception( state, descriptor, lost);
                  }
                  return true;
               }
               return false;
            };
         }
      } // handle::dispatch

      namespace pending::send::dispatch
      { 
         template< typename State, typename Lost>
         auto create( State& state, Lost lost)
         {  
            return [ &state, lost = std::move( lost)]( common::strong::file::descriptor::id fd, common::communication::select::tag::write) noexcept
            {
               Trace trace{ "gateway::group::tcp::pending::send::dispatch"};

               auto descriptor = common::strong::socket::id{ fd};
               common::log::line( verbose::log, "descriptor: ", descriptor);

               if( auto connection = state.connections.find_external( descriptor))
               {
                  try
                  {
                     connection->unsent( state.directive);
                  }
                  catch( ...)
                  {
                     detail::handle::communication::exception( state, descriptor, lost);
                  }
                  return true;
               }
               return false;
            };
         }

      } // pending::send::dispatch
      
   } // gateway::tcp
} // casual