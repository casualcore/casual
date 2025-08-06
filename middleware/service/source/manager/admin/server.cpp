//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "service/manager/admin/server.h"
#include "service/manager/transform.h"

#include "common/algorithm.h"

#include "casual/manager/service/protocol.h"


namespace casual
{
   namespace service::manager::admin
   {
      namespace local
      {
         namespace
         {
            auto state( manager::State& state)
            {
               return [ &state]( casual::manager::service::invoke::Parameter&& parameter)
               {
                  return casual::manager::service::protocol::dispatch( 
                     std::move( parameter), 
                     &transform::state, state);
               };
            }

            namespace metric
            {
               auto reset( manager::State& state)
               {
                  return [ &state]( casual::manager::service::invoke::Parameter&& parameter)
                  {
                     auto protocol = casual::manager::service::protocol::deduce( std::move( parameter));

                     auto services = protocol.extract< std::vector< std::string>>( "services");

                     return casual::manager::service::protocol::dispatch( 
                        std::move( protocol), 
                        &manager::State::metric_reset, 
                        state, 
                        std::move( services));
                  };
               }

            } // metric

         } // <unnamed>
      } // local

      std::vector< casual::manager::Service> services( manager::State& state)
      {
         return {
            casual::manager::sequential::Service{
               .name = service::name::state,
               .function = local::state( state),
               .visibility = common::service::visibility::Type::undiscoverable,
               .category = std::string{ common::service::category::admin}
            },
            casual::manager::sequential::Service{
               .name = service::name::metric::reset,
               .function = local::metric::reset( state),
               .visibility = common::service::visibility::Type::undiscoverable,
               .category = std::string{ common::service::category::admin}
            }
         };
      }

      void Policy::send_ack( const common::message::service::call::ACK& ack)
      {
         common::Trace trace{ "service::manager::admin::Policy::send_ack"};

         // we just push it to our inbound device, and let the regular handler handle it
         common::communication::ipc::inbound::device().push( ack);
      }

      void Policy::initialize( const casual::manager::service::context::State& context, manager::State& state)
      {
         common::Trace trace{ "service::manager::admin::Policy::initialize"};

         state.connect_manager( common::algorithm::transform( context.services, common::predicate::adapter::second()));
      }


   } // service::manager::admin
} // casual



