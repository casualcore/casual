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

#include "serviceframework/service/protocol.h"

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
                     return [stage=stage]( const manager::State::Request& value)
                     {
                        return admin::model::Request
                        {
                           .pid = value.process.pid,
                           .gtrid = common::transaction::id::range::global( value.trid),
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
                  return [&state]( common::service::invoke::Parameter&& parameter)
                  {
                     return serviceframework::service::user( 
                        serviceframework::service::protocol::deduce( std::move( parameter)), 
                        &detail::state, state);
                  };
               }

               auto recover( manager::State& state)
               {
                  return [&state]( common::service::invoke::Parameter&& parameter)
                  {
                     auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                     const auto gtrids = protocol.extract< std::vector< common::transaction::global::ID>>( "gtrids");
                     const auto directive = protocol.extract< model::recovery::Directive>( "directive");

                     return serviceframework::service::user( std::move( protocol), &detail::recover, state, gtrids, directive);
                  };
               }

            } // service
         } //
      } // local


      common::server::Arguments services( manager::State& state)
      {
         return 
         {{
            { 
               service::name::state,
               local::service::state( state),
               common::service::transaction::Type::none,
               common::service::visibility::Type::undiscoverable,
               common::service::category::admin
            },
            { 
               service::name::recover,
               local::service::recover( state),
               common::service::transaction::Type::none,
               common::service::visibility::Type::undiscoverable,
               common::service::category::admin
            },
         }};
      }

   } // file::manager::admin
} // casual