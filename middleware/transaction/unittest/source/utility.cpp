//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "transaction/unittest/utility.h"
#include "transaction/manager/admin/service/name.h"

#include "casual/manager/service/call.h"

#include "common/message/transaction.h"
#include "common/communication/instance.h"

namespace casual
{
   namespace transaction::unittest
   {
      namespace local
      {
         namespace
         {
            template< typename R, typename... Ts>
            R call( std::string_view service, Ts&&... arguments)
            {
               return casual::manager::service::call< R>( common::communication::instance::outbound::transaction::manager::device(), service, std::forward< Ts>( arguments)...);
            }
            
         } // <unnamed>
      } // local
      

      common::code::tx commit( const common::transaction::ID& trid)
      {
         common::message::transaction::commit::Request request{ common::process::handle()};
         request.trid = trid;

         auto reply = common::communication::ipc::call( common::communication::instance::outbound::transaction::manager::device(), request);
         return reply.state;
      }

      manager::admin::model::State state()
      {
         return local::call< manager::admin::model::State>( manager::admin::service::name::state);
      }
      
   } // transaction::unittest
   
} // casual