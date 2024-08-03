//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/strong/id.h"
#include "gateway/group/tcp.h"

#include "common/serialize/macro.h"

namespace casual
{
   namespace gateway::group::connection
   {
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
      
      
      //! Holds and manages tcp-ipc connection pairs
      template< typename Configuration>
      struct Holder
      {
         using Information = tcp::connection::Information< Configuration>;

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

         common::strong::socket::id external_descriptor( const std::string& address) const
         {
            auto is_address = [ &address]( auto& information)
            {
               return information.configuration.address == address;
            };

            if( auto found = common::algorithm::find_if( m_information, is_address))
               return found->descriptor;

            return {};
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


         void replace_configuration( Configuration configuration)
         {
            auto is_address = [ &configuration]( auto& information)
            {
               return information.configuration.address == configuration.address;
            };

            if( auto found = common::algorithm::find_if( m_information, is_address))
               found->configuration = std::move( configuration);
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

         //! Destruct and remove all associated state to `descriptor` and
         //! @returns the `Information` about the external connection (could be
         //! used to try to reconnect, or log)
         Information extract( 
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
               result.protocol = pair.second.protocol();
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

         std::vector< Configuration> configuration() const
         {
            return common::algorithm::transform( m_information, []( auto& information){ return information.configuration;});
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
         std::unordered_map< common::strong::socket::id, tcp::Connection> m_external;
         std::unordered_map< common::strong::ipc::descriptor::id, common::communication::ipc::inbound::Device> m_internal;

         std::vector< Information> m_information;
         tcp::logical::connect::Pending< Configuration> m_pending;
      };

      
   } // gateway::group::connection
} // casual