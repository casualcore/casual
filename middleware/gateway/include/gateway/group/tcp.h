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

      struct Connection
      {
         using complete_type = common::communication::tcp::message::Complete;

         inline explicit Connection( common::communication::tcp::Duplex&& device, message::protocol::Version protocol)
            : m_device{ std::move( device)}, m_protocol{ protocol} {}

      
         inline auto descriptor() const noexcept { return m_device.connector().descriptor();}

         //! tries to send as much of unsent as possible, if any.
         void unsent( common::communication::select::Directive& directive);
         
         template< typename M>
         auto send( common::communication::select::Directive& directive, M&& message)
         {
            return send( directive, common::serialize::native::complete< complete_type>( std::forward< M>( message)));
         }

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
         
         common::strong::correlation::id send( common::communication::select::Directive& directive, complete_type&& complete);

         std::vector< complete_type> m_unsent;
         common::communication::tcp::Duplex m_device;
         message::protocol::Version m_protocol;
      };

      template< typename Configuration>
      struct External
      {
         struct Information
         {
            Information( common::strong::file::descriptor::id descriptor,
               common::domain::Identity domain,
               Configuration configuration)
               : descriptor{ descriptor}, domain{ std::move( domain)}, configuration{ std::move( configuration)}
            {}

            common::strong::file::descriptor::id descriptor;
            common::domain::Identity domain;
            Configuration configuration;
            platform::time::point::type created = platform::time::clock::type::now();

            inline friend bool operator == ( const Information& lhs, common::strong::file::descriptor::id rhs) { return lhs.descriptor == rhs;} 

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( descriptor);
               CASUAL_SERIALIZE( domain);
               CASUAL_SERIALIZE( configuration);
               CASUAL_SERIALIZE( created);
            )
         };

         auto connected( 
            common::communication::select::Directive& directive,
            const gateway::message::domain::Connected& message)
         {
            auto connector = m_pending.extract( message.connector);
            // we 'know' the process has/going to exit by it self. We make sure we don't try to send signals
            connector.process.clear();
            
            auto descriptor = connector.socket.descriptor();

            common::log::line( common::log::category::information, "connection established to domain: '", message.domain.name, 
               "' - host: ", common::communication::tcp::socket::address::host( connector.socket),
               ", peer: ", common::communication::tcp::socket::address::peer( connector.socket));

            m_connections.emplace_back( std::move( connector.socket), message.version);
            m_information.emplace_back( descriptor, message.domain, std::move( connector.configuration));
            directive.read.add( descriptor);

            return descriptor;
         }

         auto descriptors() const noexcept 
         {
            return common::algorithm::transform( m_connections, []( auto& connection){ return connection.descriptor();});
         }

         group::tcp::Connection* connection( common::strong::file::descriptor::id descriptor)
         {
            if( auto found = common::algorithm::find( m_connections, descriptor))
               return found.data();
            return nullptr;
         }

         const Information* information( common::strong::file::descriptor::id descriptor) const noexcept
         {
            if( auto found = common::algorithm::find( m_information, descriptor))
               return found.data();
            return nullptr;
         }

         Configuration remove( 
            common::communication::select::Directive& directive, 
            common::strong::file::descriptor::id descriptor)
         {
            casual::assertion( descriptor, "descriptor: ", descriptor);

            directive.read.remove( descriptor);
            common::algorithm::container::trim( m_connections, common::algorithm::remove( m_connections, descriptor));
            
            auto found = common::algorithm::find( m_information, descriptor);
            casual::assertion( descriptor, "fail to find information for descriptor: ", descriptor);
               
            return common::algorithm::container::extract( m_information, std::begin( found)).configuration;
         }

         void clear( common::communication::select::Directive& directive)
         {
            directive.read.remove( descriptors());
            m_connections.clear();
            m_information.clear();
         }

         auto& information() const noexcept { return m_information;}
         auto& connections() const noexcept { return m_connections;}
         auto connections() noexcept { return common::range::make( m_connections);}

         auto& pending() const noexcept { return m_pending;}
         auto& pending()  noexcept { return m_pending;}

         void last( common::strong::file::descriptor::id value) { m_last = value;}
         auto last() const noexcept { return m_last;}

         auto empty() const noexcept { return m_connections.empty();}

         //! create a state reply and fill it with connections
         template< typename M>
         auto reply( M&& request) const noexcept
         {
            auto reply = common::message::reverse::type( request, common::process::handle());

            using Connection = common::traits::iterable::value_t< decltype( reply.state.connections)>;

            reply.state.connections = common::algorithm::transform( m_connections, [&]( auto& connection)
            {
               auto descriptor = connection.descriptor();
               Connection result;
               result.runlevel = decltype( result.runlevel)::connected;
               result.descriptor = descriptor;
               result.address.local = common::communication::tcp::socket::address::host( descriptor);
               result.address.peer = common::communication::tcp::socket::address::peer( descriptor);

               if( auto found = common::algorithm::find( m_information, descriptor))
               {
                  result.domain = found->domain;
                  result.configuration = found->configuration;
                  result.created = found->created;
               }

               return result;
            });

            common::algorithm::transform( m_pending.connections(), std::back_inserter( reply.state.connections), []( auto& connection)
            {
               Connection result;
               result.runlevel = decltype( result.runlevel)::connected;
               result.address.peer = connection.configuration.address;
               return result;
            });

            return reply;
         }


         CASUAL_LOG_SERIALIZE( 
            CASUAL_SERIALIZE_NAME( m_connections, "connections");
            CASUAL_SERIALIZE_NAME( m_information, "information");
            CASUAL_SERIALIZE_NAME( m_pending, "pending");
            CASUAL_SERIALIZE_NAME( m_last, "last");
         )
      private:

         std::vector< Connection> m_connections;
         std::vector< Information> m_information;
         logical::connect::Pending< Configuration> m_pending;

         //! holds the last external connection that was used
         common::strong::file::descriptor::id m_last;
      };

     namespace handle::dispatch
     {
         template< typename State, typename Handler, typename Lost>
         auto create( State& state, Handler handler, Lost lost)
         {
            using namespace common;

            return [ &state, handler = std::move( handler), lost = std::move( lost)]( strong::file::descriptor::id descriptor, communication::select::tag::read) mutable
            {
               constexpr auto is_outbound_v = std::is_same_v< decltype( lost( state, descriptor)), decltype( message::outbound::connection::Lost{}.configuration)>;

               if( auto connection = state.external.connection( descriptor))
               {
                  try
                  {
                     // we need to handle outbound and inbound differently (should be the same?)
                     if constexpr( is_outbound_v)
                     {
                        state.external.last( descriptor);
                        handler( connection->next());
                     }
                     else
                     {
                        if( auto correlation = handler( connection->next()))
                        {
                           if( ! algorithm::find( state.correlations, correlation))
                              state.correlations.emplace_back( std::move( correlation), descriptor);
                        }
                     }
                  }
                  catch( ...)
                  {
                     auto error = exception::capture();
                     if( error.code() != code::casual::communication_unavailable)
                     {
                        auto information = state.external.information( descriptor);
                        log::line( log::category::error, "failed to receive from: ", information->domain.name, ", configured address: ", information->configuration.address, " - error: ", error);
                     }

                     // we 'lost' the connection in some way - we put a connection::Lost on our own ipc-device, and handle it
                     // later (and differently depending on if we're 'regular' or 'reversed')
                     // we staticly decide witch message is appropriate.

                     if constexpr( is_outbound_v)
                        communication::ipc::inbound::device().push( 
                           message::outbound::connection::Lost{ lost( state, descriptor)});
                     else
                        communication::ipc::inbound::device().push( 
                           message::inbound::connection::Lost{ lost( state, descriptor)});
                  }
                  return true;
               }
               return false;
            };
         }
     } // handle::dispatch

      namespace pending::send
      {
         template< typename State>
         auto dispatch( State& state)
         {
            return [&state]( common::strong::file::descriptor::id descriptor, common::communication::select::tag::write)
            {
               Trace trace{ "gateway::group::tcp::pending::send::dispatch"};
               common::log::line( verbose::log, "descriptor: ", descriptor);

               if( auto connection = state.external.connection( descriptor))
               {
                  connection->unsent( state.directive);
                  return true;
               }

               return false;

            };
         }
      } // pending::send
      
   } // gateway::tcp
} // casual