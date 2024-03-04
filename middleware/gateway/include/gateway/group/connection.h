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


      struct Connections
      {

         inline common::strong::ipc::descriptor::id other( common::strong::socket::id tcp) const noexcept
         {
            if( auto found = common::algorithm::find( m_mapping, tcp))
               return found->ipc;
            return {};
         }

         inline common::strong::socket::id other( common::strong::ipc::descriptor::id ipc) const noexcept
         {
            if( auto found = common::algorithm::find( m_mapping, ipc))
               return found->tcp;
            return {};
         }

         tcp::Connection* external( common::strong::socket::id tcp) noexcept;
         tcp::Connection* external( common::strong::ipc::descriptor::id ipc) noexcept;
         common::communication::ipc::inbound::Device* internal( common::strong::socket::id tcp) noexcept;
         common::communication::ipc::inbound::Device* internal( common::strong::ipc::descriptor::id ipc) noexcept;

         void remove( common::strong::socket::id tcp);

         std::vector< common::strong::socket::id> external_descriptors() const;
         std::vector< common::strong::ipc::descriptor::id> internal_descriptors() const;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( m_mapping);
            CASUAL_SERIALIZE( m_external);
            CASUAL_SERIALIZE( m_internal);
         )

      private:
         
         std::vector< descriptor::Pair> m_mapping;
         std::unordered_map< common::strong::socket::id, tcp::Connection> m_external;
         std::unordered_map< common::strong::ipc::descriptor::id, common::communication::ipc::inbound::Device> m_internal;
         
      };

      
   } // gateway::group::connection
} // casual