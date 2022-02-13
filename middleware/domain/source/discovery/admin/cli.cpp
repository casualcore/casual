//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/discovery/admin/cli.h"

#include "domain/discovery/api.h"

#include "common/terminal.h"
#include "common/communication/ipc.h"

namespace casual
{
   using namespace common;

   namespace domain::discovery::admin
   {
      namespace local
      {
         namespace
         {
            namespace rediscovery
            {
               auto option()
               {
                  auto invoke = []()
                  {
                     auto correlation = discovery::rediscovery::request();
                     if( ! terminal::output::directive().block())
                     {
                        communication::ipc::inbound::device().discard( correlation);
                        return;
                     }

                     communication::ipc::receive< message::discovery::api::Reply>( correlation);
                  };

                  return argument::Option{
                     std::move( invoke),
                     { "--rediscover"},
                     "rediscover all 'discoverable' agents"
                  };
               }
            } // rediscovery
         } // <unnamed>
      } // local

      struct cli::Implementation
      {
         argument::Group options()
         {
            return { [](){}, { "discovery"}, "responsible for discovery stuff",
               local::rediscovery::option()
            };
         }

      };

      cli::cli() = default;
      cli::~cli() = default;

      argument::Group cli::options() &
      {
         return m_implementation->options();
      }

   } // domain::discovery::admin
} // casual
