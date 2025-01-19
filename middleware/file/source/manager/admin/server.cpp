//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "file/manager/admin/server.h"
#include "file/manager/admin/services.h"
#include "file/manager/admin/model.h"

#include "file/manager/handle.h"

#include "common/transaction/id.h"

#include "serviceframework/service/protocol.h"


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
                  admin::model::State state( const manager::State& state)
                  {
                     auto transform = []( const message::reserve::Request& value)
                     {
                        const auto trid = common::transaction::id::range::data( value.trid);

                        return admin::model::Request
                        {
                           .process = value.process,
                           .trid = platform::binary::type{ std::begin( trid), std::end( trid)},
                           .path = value.path,
                        };
                     };

                     admin::model::State result;

                     std::ranges::transform( state.working, std::back_inserter( result.working), transform);
                     std::ranges::transform( state.pending, std::back_inserter( result.pending), transform);

                     return result;
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
         }};
      }

   } // file::manager::admin
} // casual