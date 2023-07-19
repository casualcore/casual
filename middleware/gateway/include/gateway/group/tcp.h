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
            inline friend bool operator == ( const Information& lhs, const common::strong::domain::id& rhs) { return lhs.domain == rhs;} 
            
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
            
            // no need for children to inherit the file descriptor from this point onwards.
            connector.socket.set( common::communication::socket::option::File::close_in_child);
            
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

         const Information* information( const common::strong::domain::id& domain) const noexcept
         {
            if( auto found = common::algorithm::find( m_information, domain))
               return found.data();
            return nullptr;
         }
         // TODO should be named extract.
         Information remove( 
            common::communication::select::Directive& directive, 
            common::strong::file::descriptor::id descriptor)
         {
            casual::assertion( descriptor, "descriptor: ", descriptor);

            // make sure we remove from read and write
            directive.remove( descriptor);
            common::algorithm::container::trim( m_connections, common::algorithm::remove( m_connections, descriptor));
            
            auto found = common::algorithm::find( m_information, descriptor);
            casual::assertion( found, "fail to find information for descriptor: ", descriptor);
               
            return common::algorithm::container::extract( m_information, std::begin( found));
         }

         void clear( common::communication::select::Directive& directive)
         {
            directive.remove( descriptors());
            m_connections.clear();
            m_information.clear();
         }

         auto& information() const noexcept { return m_information;}
         auto& connections() const noexcept { return m_connections;}

         auto& pending() const noexcept { return m_pending;}
         auto& pending() noexcept { return m_pending;}

         void last( common::strong::file::descriptor::id value) { m_last = value;}
         auto last() const noexcept { return m_last;}

         auto empty() const noexcept { return m_connections.empty();}

         //! create a state reply and fill it with connections
         template< typename M>
         auto reply( M&& request) const noexcept
         {
            auto reply = common::message::reverse::type( request, common::process::handle());

            using Connection = std::ranges::range_value_t< decltype( reply.state.connections)>;

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

      namespace detail::handle::communication
      {
         template< typename State, typename Lost>
         void exception( State& state, common::strong::file::descriptor::id descriptor, Lost lost) noexcept
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
                  auto information = casual::assertion( state.external.information( descriptor), "failed to find information for descriptor: ", descriptor);
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
            return [ &state, handler = std::move( handler), lost = std::move( lost)]( common::strong::file::descriptor::id descriptor, common::communication::select::tag::read) mutable
            {
               if( auto connection = state.external.connection( descriptor))
               {
                  try
                  {
                     state.external.last( descriptor);

                     auto count = Policy::next::tcp();
                     while( count-- > 0 && handler( connection->next()))
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
            return [ &state, lost = std::move( lost)]( common::strong::file::descriptor::id descriptor, common::communication::select::tag::write) noexcept
            {
               Trace trace{ "gateway::group::tcp::pending::send::dispatch"};
               common::log::line( verbose::log, "descriptor: ", descriptor);

               if( auto connection = state.external.connection( descriptor))
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