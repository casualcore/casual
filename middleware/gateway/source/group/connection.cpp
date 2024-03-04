//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/connection.h"

namespace casual
{
   using namespace common;

   namespace gateway::group::connection
   {
      void Connections::remove( common::strong::socket::id tcp)
      {
         if( auto mapped = algorithm::find( m_mapping, tcp))
         {
            m_internal.erase( mapped->ipc);
            algorithm::container::erase( m_mapping, std::begin( mapped));
         }
         m_external.erase( tcp);
      }

      tcp::Connection* Connections::external( common::strong::socket::id tcp) noexcept
      {
         if( auto found = algorithm::find( m_external, tcp))
            return &found->second;
         
         return nullptr;
      }

      tcp::Connection* Connections::external( common::strong::ipc::descriptor::id ipc) noexcept
      {
         if( auto mapped = algorithm::find( m_mapping, ipc))
            if( auto found = algorithm::find( m_external, mapped->tcp))
               return &found->second;

         return nullptr;
      }

      common::communication::ipc::inbound::Device* Connections::internal( common::strong::socket::id tcp) noexcept
      {
         if( auto mapped = algorithm::find( m_mapping, tcp))
            if( auto found = algorithm::find( m_internal, mapped->ipc))
               return &found->second;

         return nullptr;
      }

      common::communication::ipc::inbound::Device* Connections::internal( common::strong::ipc::descriptor::id ipc) noexcept
      {
         if( auto found = algorithm::find( m_internal, ipc))
            return &found->second;

         return nullptr;
      }

      std::vector< common::strong::socket::id> Connections::external_descriptors() const
      {
         return algorithm::transform( m_external, []( auto& pair){ return pair.first;});
      }

      std::vector< common::strong::ipc::descriptor::id> Connections::internal_descriptors() const
      {
         return algorithm::transform( m_internal, []( auto& pair){ return pair.first;});
      }


      
   } // gateway::group::connection
} // casual