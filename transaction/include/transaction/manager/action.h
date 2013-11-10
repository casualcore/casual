//!
//! manager_action.h
//!
//! Created on: Aug 14, 2013
//!     Author: Lazan
//!

#ifndef MANAGER_ACTION_H_
#define MANAGER_ACTION_H_

#include "transaction/manager/state.h"

#include "common/environment.h"
#include "common/trace.h"

namespace casual
{
   namespace transaction
   {
      namespace action
      {

         template< typename BQ, typename RQ>
         void configure( State& state, BQ& brokerWriter, RQ& receiveQueue)
         {
            {
               common::trace::Exit trace( "transaction manager connect to broker");

               //
               // Do the initialization dance with the broker
               //
               common::message::transaction::Connect connect;

               connect.path = common::environment::file::executable();
               connect.server.queue_id = receiveQueue.ipc().id();

               brokerWriter( connect);
            }

            {
               common::trace::Exit trace( "transaction manager configure");

               //
               // Wait for configuration
               //
               common::message::transaction::Configuration configuration;
               receiveQueue( configuration);

               //
               // configure state
               //
               state::configure( state, configuration);
            }

         }



         namespace boot
         {
            struct Proxie : state::Base
            {
               using state::Base::Base;

               void operator () ( const state::resource::Proxy& proxy);
            };
         } // boot



      } // action


   } // transaction


} // casual

#endif // MANAGER_ACTION_H_
