//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "gateway/manager/admin/server.h"
#include "gateway/manager/admin/model.h"
#include "gateway/manager/handle.h"
#include "gateway/transform.h"


#include "serviceframework/service/protocol.h"


#include "xatmi.h"

namespace casual
{
   using namespace common;
   namespace gateway::manager::admin
   {
      namespace local
      {
         namespace
         {
            namespace service
            {
               auto state( manager::State& state)
               {
                  return [&state]( common::service::invoke::Parameter&& parameter)
                  {
                     Trace trace{ "gateway::manager::admin::local::service::state"};

                     auto get_state = []( auto& state)
                     {
                        auto request_state = []( auto&& range, auto&& message)
                        {
                           return algorithm::transform( range, [&message]( auto& reverse)
                           {
                              return communication::device::async::call( reverse.process.ipc, message);
                           });
                        };

                        auto is_reverse = []( auto& bound){ return bound.configuration.connect == decltype( bound.configuration.connect)::reversed;};
                        auto is_regular = predicate::negate( is_reverse);

                        auto inbounds = request_state( algorithm::filter( state.inbound.groups, is_regular), message::inbound::state::Request{ process::handle()});
                        auto reverse_inbounds = request_state( algorithm::filter( state.inbound.groups, is_reverse), message::inbound::reverse::state::Request{ process::handle()});

                        auto outbounds = request_state( algorithm::filter( state.outbound.groups, is_regular), message::outbound::state::Request{ process::handle()});
                        auto reverse_outbounds = request_state( algorithm::filter( state.outbound.groups, is_reverse), message::outbound::reverse::state::Request{ process::handle()});

                        auto get_reply = []( auto& future){ return future.get( manager::ipc::inbound());};

                        return gateway::transform::state( state, 
                           std::make_tuple( algorithm::transform( inbounds, get_reply), algorithm::transform( reverse_inbounds, get_reply)),
                           std::make_tuple( algorithm::transform( outbounds, get_reply), algorithm::transform( reverse_outbounds, get_reply)));
                     };

                     return serviceframework::service::user( 
                        std::move( parameter),
                        get_state,
                        state);
                  };
               }

            }


         } // <unnamed>
      } // local

      common::server::Arguments services( manager::State& state)
      {
         return { {
               { service::name::state,
                  local::service::state( state),
                  common::service::transaction::Type::none,
                  common::service::category::admin
               }
         }};
      }
   } // gateway::manager::admin
} // casual
