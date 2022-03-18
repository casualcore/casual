//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/process.h"
#include "common/communication/socket.h"
#include "common/algorithm.h"
#include "common/algorithm/container.h"
#include "common/code/raise.h"
#include "common/code/casual.h"

#include <optional>
#include <string_view>

namespace casual
{
   //! Holds "all" stuff related to logical connection phase
   namespace gateway::group::tcp::logical::connect
   {

      enum class Bound : std::uint8_t
      {
         in,
         out
      };

      constexpr std::string_view description( Bound value) noexcept;


      common::Process spawn( Bound bound, const common::communication::Socket& socket);

      template< typename Configuration>
      struct Pending
      {
         struct Connector
         {
            Connector( common::Process process, common::communication::Socket socket, Configuration configuration)
               : process{ std::move( process)}, socket{ std::move( socket)}, configuration{ std::move( configuration)}
            {}

            common::Process process;
            common::communication::Socket socket;
            Configuration configuration;

            friend bool operator == ( const Connector& lhs, common::strong::process::id rhs) { return lhs.process.pid == rhs;}

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( process);
               CASUAL_SERIALIZE( socket);
               CASUAL_SERIALIZE( configuration);
            )
         };

         void add( common::Process&& connector,
            common::communication::Socket&& socket,
            Configuration configuration)
         {
            m_connectors.emplace_back( std::move( connector), std::move( socket), std::move( configuration));
         }

         auto extract( common::strong::process::id pid)
         {
            if( auto found = common::algorithm::find( m_connectors, pid))
               return common::algorithm::container::extract( m_connectors, std::begin( found));

            common::code::raise::error( common::code::casual::invalid_semantics, "failed to correlate pending connector for pid: ", pid);
         }

         //! removes the connection iff we've got it, and the exit is NOT
         //! a clean exit (status 0)
         //! @returns configuration for the connector if the connection failed, otherwise empty.
         std::optional< Configuration> exit( const common::process::lifetime::Exit& exit)
         {
            if( exit.reason == decltype( exit.reason)::exited && exit.status == 0)
               return {};

            if( auto found = common::algorithm::find( m_connectors, exit.pid))
            {
               auto connector = common::algorithm::container::extract( m_connectors, std::begin( found));
               // we 'clear' the connector to not send unnecessary signals.
               connector.process.clear();
               return { std::move( connector.configuration)};
            }
            return {};
         }

         auto& connections() const noexcept { return m_connectors;}

         CASUAL_LOG_SERIALIZE( 
            CASUAL_SERIALIZE_NAME( m_connectors, "connectors");
         )

      private:
         std::vector< Connector> m_connectors;

      };

   } // gateway::group::tcp::logical::connect
} // casual