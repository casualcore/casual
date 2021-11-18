//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/manager/admin/server.h"
#include "gateway/manager/admin/model.h"

#include "serviceframework/service/protocol/call.h"

#include "common/process.h"
#include "common/algorithm.h"

namespace casual
{
   namespace test::unittest::gateway
   {
      namespace state
      {
         inline auto fetch()
         {
            serviceframework::service::protocol::binary::Call call;
            auto reply = call( casual::gateway::manager::admin::service::name::state);
            return reply.extract< casual::gateway::manager::admin::model::State>();
         }


         template< typename P>
         auto until( P&& predicate)
         {
            auto state = fetch();
            auto count = 500;

            while( ! predicate( state) && count-- > 0)
            {
               common::process::sleep( std::chrono::milliseconds{ 2});
               state = fetch();
            }

            return state;
         }

         namespace predicate::outbound
         {
            namespace detail
            {
               inline auto connected()
               {
                  return []( auto& connection)
                  {
                     return connection.bound != decltype( connection.bound)::out || connection.remote.id;
                  };
               }
            } // detail

            // returns a predicate that checks if all out-connections has a 'remote id'
            inline auto connected() 
            {
               return []( auto& state)
               {
                  return common::algorithm::all_of( state.connections, detail::connected());  
               };
            };

            // returns a predicate that checks if `count` out-connections has a 'remote id'
            inline auto connected( platform::size::type count)
            {
               return [count]( auto& state)
               {
                  return common::algorithm::count_if( state.connections, detail::connected()) == count;
               };
            }

            // returns a predicate that checks if all out-connections has NOT a 'remote id'
            inline auto disconnected()
            {
               return []( auto& state)
               {
                  return common::algorithm::all_of( state.connections, []( auto& connection)
                  {
                     return connection.bound != decltype( connection.bound)::out || ! connection.remote.id;
                  }); 
               };
            };
         } // predicate::outbound
      } // state

   } // test::unittest::gateway
} // casual