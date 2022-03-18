//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/tcp/logical/connect.h"

#include "common/string/compose.h"

namespace casual
{
   using namespace common;

   //! Holds "all" stuff related to logical connection phase
   namespace gateway::group::tcp::logical::connect
   {

      constexpr std::string_view description( Bound value) noexcept
      {
         switch( value)
         {
            case Bound::in: return "in";
            case Bound::out: return "out";
         }
         return "<unknown>";
      }

      common::Process spawn( Bound bound, const common::communication::Socket& socket)
      {
         auto path = process::path().parent_path() / "casual-gateway-group-tcp-logical-connect";

         return Process{ path, {
            "--descriptor", std::to_string( socket.descriptor().value()),
            "--ipc", string::compose( process::handle().ipc),
            "--bound", string::compose( bound)
         }};
      }

   } // gateway::group::tcp::logical::connect
} // casual
