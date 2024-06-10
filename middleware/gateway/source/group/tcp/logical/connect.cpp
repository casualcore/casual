//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/tcp/logical/connect.h"
#include "gateway/message/protocol.h"

#include "common/string/compose.h"


namespace casual
{
   using namespace common;

   //! Holds "all" stuff related to logical connection phase
   namespace gateway::group::tcp::logical::connect
   {
      namespace local
      {
         namespace
         {
            auto path()
            {
               if constexpr( message::protocol::compiled_for_version() == message::protocol::Version::v1_2)
                  return process::path().parent_path() / "casual-gateway-group-tcp-logical-connect.1.2";
               else
                  return process::path().parent_path() / "casual-gateway-group-tcp-logical-connect";

            }
         } // <unnamed>
      } // local

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
         return Process{ local::path(), {
            "--descriptor", std::to_string( socket.descriptor().value()),
            "--ipc", string::compose( process::handle().ipc),
            "--bound", string::compose( bound)
         }};
      }

   } // gateway::group::tcp::logical::connect
} // casual
