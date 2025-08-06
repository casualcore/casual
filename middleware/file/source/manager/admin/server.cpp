//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "file/manager/admin/server.h"
#include "file/manager/admin/services.h"
#include "file/manager/admin/model.h"

#include "file/manager/handle.h"

#include "file/manager/resource.h"

#include "common/transaction/id.h"

#include "casual/manager/service/protocol.h"

#include <ranges>

namespace casual
{
   namespace file::manager::admin
   {
      namespace local
      {
         namespace
         {
            namespace service
            {
               namespace detail
               {
                  auto transform( const admin::model::Stage stage)
                  {
                     return [ stage]( const manager::State::Request& value)
                     {
                        return admin::model::Request
                        {
                           .pid = value.process.pid,
                           .gtrid = common::transaction::global::ID{ value.trid.global()},
                           .stage = stage,
                           .path = value.path,
                           .time = value.time,
                        };
                     };
                  }

                  auto state( const manager::State& state)
                  {
                     admin::model::State result;

                     std::ranges::transform( state.working, std::back_inserter( result.requests), transform( admin::model::Stage::working));
                     std::ranges::transform( state.pending, std::back_inserter( result.requests), transform( admin::model::Stage::pending));

                     return result;
                  }

                  std::vector< common::transaction::global::ID> recover( manager::State& state, const std::vector< common::transaction::global::ID>& gtrids, const model::recovery::Directive directive)
                  {
                     switch( directive)
                     {
                     case model::recovery::Directive::commit:
                        return resource::recovery::commit( state, gtrids);
                     case model::recovery::Directive::rollback:
                        return resource::recovery::rollback( state, gtrids);
                     default:
                        return {};
                     }
                  }

               } // detail

               auto state( const manager::State& state)
               {
                  return [&state]( casual::manager::service::invoke::Parameter&& parameter)
                  {
                     return casual::manager::service::protocol::dispatch(
                        std::move( parameter), 
                        &detail::state, state);
                  };
               }

               auto recover( manager::State& state)
               {
                  return [&state]( casual::manager::service::invoke::Parameter&& parameter)
                  {
                     auto protocol = casual::manager::service::protocol::deduce( std::move( parameter));

                     const auto gtrids = protocol.extract< std::vector< common::transaction::global::ID>>( "gtrids");
                     const auto directive = protocol.extract< model::recovery::Directive>( "directive");

                     return casual::manager::service::protocol::dispatch( std::move( protocol), &detail::recover, state, gtrids, directive);
                  };
               }

            } // service
         } //
      } // local


      std::vector< casual::manager::Service> services( manager::State& state)
      {
         return {
            casual::manager::sequential::Service{ 
               .name = service::name::state,
               .function = local::service::state( state),
               .visibility = common::service::visibility::Type::undiscoverable,
               .category = std::string{ common::service::category::admin}
            },
            casual::manager::sequential::Service{
               .name = service::name::recover,
               .function = local::service::recover( state),
               .visibility = common::service::visibility::Type::undiscoverable,
               .category = std::string{ common::service::category::admin}
            }
         };
      }

   } // file::manager::admin
} // casual