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
         if( auto connection = state.external.find_external( descriptor))
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

         namespace descriptor
         {
            struct Pair
            {
               common::strong::ipc::descriptor::id ipc;
               common::strong::socket::id tcp;

               inline friend bool operator == ( const Pair& lhs, common::strong::ipc::descriptor::id rhs) { return lhs.ipc == rhs;}
               inline friend bool operator == ( const Pair& lhs, common::strong::socket::id rhs) { return lhs.tcp == rhs;}

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( ipc);
                  CASUAL_SERIALIZE( tcp);
               );
            };
            
         } // descriptor
         
      } // connection

      template< typename Configuration>
      struct External
      {
         using Information = connection::Information< Configuration>;

         auto connected( 
            common::communication::select::Directive& directive,
            const gateway::message::domain::Connected& message)
         {
            auto connector = m_pending.extract( message.connector);
            
            // we 'know' the process has/going to exit by it self. We make sure we don't try to send signals
            connector.process.clear();
            
            // no need for children to inherit the file descriptor from this point onwards.
            connector.socket.set( common::communication::socket::option::File::close_in_child);
            
            auto tcp_descriptor = connector.socket.descriptor();

            common::log::line( common::log::category::information, "connection established to domain: '", message.domain.name, 
               "' - host: ", common::communication::tcp::socket::address::host( connector.socket),
               ", peer: ", common::communication::tcp::socket::address::peer( connector.socket));

            {
               common::communication::ipc::inbound::Device inbound;
               auto ipc_descriptor = inbound.descriptor();

               m_mapping.push_back( connection::descriptor::Pair{ ipc_descriptor, tcp_descriptor});
               m_internal.emplace( ipc_descriptor, std::move( inbound));

               directive.read.add( ipc_descriptor);
            }

            {
               m_external.emplace( tcp_descriptor, tcp::Connection{ std::move( connector.socket), message.version});
               m_information.emplace_back( tcp_descriptor, message.domain, std::move( connector.configuration));
               
               directive.read.add( tcp_descriptor);
            }

            return common::range::back( m_mapping);
         }

         inline std::vector< common::strong::socket::id> external_descriptors() const
         {
            return common::algorithm::transform( m_mapping, []( auto& pair){ return pair.tcp;});
         }

         inline std::vector< common::strong::ipc::descriptor::id> internal_descriptors() const
         {
            return common::algorithm::transform( m_mapping, []( auto& pair){ return pair.ipc;});
         }

         inline common::strong::ipc::descriptor::id partner( common::strong::socket::id tcp) const noexcept
         {
            if( auto found = common::algorithm::find( m_mapping, tcp))
               return found->ipc;
            return {};
         }

         inline common::strong::socket::id partner( common::strong::ipc::descriptor::id ipc) const noexcept
         {
            if( auto found = common::algorithm::find( m_mapping, ipc))
               return found->tcp;
            return {};
         }

         inline tcp::Connection* find_external( common::strong::socket::id tcp) noexcept
         {
            if( auto found = common::algorithm::find( m_external, tcp))
               return &found->second;

            return nullptr;
         }

         inline tcp::Connection* find_external( common::strong::ipc::descriptor::id ipc) noexcept
         {
            if( auto mapped = common::algorithm::find( m_mapping, ipc))
               return find_external( mapped->tcp);

            return nullptr;
         }
         

         const Information* information( common::strong::socket::id descriptor) const noexcept
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

         inline common::communication::ipc::inbound::Device* find_internal( common::strong::ipc::descriptor::id ipc) noexcept
         {
            if( auto found = common::algorithm::find( m_internal, ipc))
               return &found->second;

            return nullptr;
         }

         inline common::communication::ipc::inbound::Device* find_internal( common::strong::socket::id tcp) noexcept
         {
            if( auto mapped = common::algorithm::find( m_mapping, tcp))
               return find_internal( mapped->ipc);

            return nullptr;
         }

         inline common::process::Handle process_handle( common::strong::ipc::descriptor::id ipc) const noexcept
         {
            if( auto found = common::algorithm::find( m_internal, ipc))
               return common::process::Handle{ common::process::id(), found->second.connector().handle().ipc()};

            common::log::error( common::code::casual::internal_correlation, "failed to find  ", ipc);
            return {};
         }

         inline common::process::Handle process_handle( common::strong::socket::id tcp) const noexcept
         {
            if( auto found = common::algorithm::find( m_mapping, tcp))
               return process_handle( found->ipc);
            return {};
         }

         // TODO should be named extract.
         Information remove( 
            common::communication::select::Directive& directive, 
            common::strong::socket::id descriptor)
         {
            casual::assertion( descriptor, "descriptor: ", descriptor);

            // make sure we remove the ipc partner.
            auto ipc = partner( descriptor);
            common::algorithm::container::erase( m_internal, ipc);
            // make sure we remove from read directive (ipc only reads)
            directive.read.remove( ipc);

            // make sure we remove from read and write
            directive.remove( descriptor);
            common::algorithm::container::erase( m_external, descriptor);

            common::algorithm::container::erase( m_mapping, descriptor);
            
            auto found = common::algorithm::find( m_information, descriptor);
            casual::assertion( found, "fail to find information for descriptor: ", descriptor);
               
            return common::algorithm::container::extract( m_information, std::begin( found));
         }

         void clear( common::communication::select::Directive& directive)
         {
            directive.remove( external_descriptors());
            directive.remove( internal_descriptors());
            m_external.clear();
            m_internal.clear();
         }

         auto& information() const noexcept { return m_information;}
         auto& external() const noexcept { return m_external;}
         auto& internal() const noexcept { return m_internal;}

         auto& pending() const noexcept { return m_pending;}
         auto& pending() noexcept { return m_pending;}

         auto empty() const noexcept { return m_mapping.empty();}

         //! create a state reply and fill it with connections
         template< typename M>
         auto reply( M&& request) const noexcept
         {
            auto reply = common::message::reverse::type( request, common::process::handle());

            using Connection = std::ranges::range_value_t< decltype( reply.state.connections)>;

            reply.state.connections = common::algorithm::transform( m_external, [&]( auto& pair)
            {
               auto descriptor = pair.first;
               Connection result;
               result.runlevel = decltype( result.runlevel)::connected;
               result.descriptor = descriptor;
               result.address.local = common::communication::tcp::socket::address::host( descriptor);
               result.address.peer = common::communication::tcp::socket::address::peer( descriptor);
               
               if( auto descriptors = common::algorithm::find( m_mapping, descriptor))
                  if( auto pair = common::algorithm::find( m_internal, descriptors->ipc))
                     result.ipc = pair->second.connector().handle().ipc();

               if( auto found = information( descriptor))
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
            CASUAL_SERIALIZE( m_mapping);
            CASUAL_SERIALIZE( m_external);
            CASUAL_SERIALIZE( m_internal);
            CASUAL_SERIALIZE( m_information);
            CASUAL_SERIALIZE( m_pending);
         )
      private:

         std::vector< connection::descriptor::Pair> m_mapping;
         std::unordered_map< common::strong::socket::id, Connection> m_external;
         std::unordered_map< common::strong::ipc::descriptor::id, common::communication::ipc::inbound::Device> m_internal;

         std::vector< Information> m_information;
         logical::connect::Pending< Configuration> m_pending;
      };

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
            return [ &state, handler = std::move( handler), lost = std::move( lost)]( common::strong::file::descriptor::id fd, common::communication::select::tag::read) mutable
            {
               // we know the descriptor is a socket descriptor, we convert.
               auto descriptor = common::strong::socket::id{ fd};

               if( auto connection = state.external.find_external( descriptor))
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

               if( auto connection = state.external.find_external( descriptor))
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