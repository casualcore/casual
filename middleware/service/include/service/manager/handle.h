//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "service/manager/state.h"

#include "common/message/dispatch.h"
#include "common/message/gateway.h"
#include "common/message/transaction.h"
#include "common/message/event.h"

#include "common/server/handle/call.h"
#include "common/server/context.h"




namespace casual
{

   namespace service
   {
      namespace manager
      {

         namespace ipc
         {
            common::communication::ipc::inbound::Device& device();
         } // ipc

         namespace handle
         {
            namespace metric
            {
               //! tries to send metrics regardless
               void send( State& state);

               namespace batch
               {
                  //! send metrics if we've reach batch-limit
                  void send( State& state);
               } // batch
               
            } // metric

            namespace process
            {
               void exit( const common::process::lifetime::Exit& exit);   
            } // process

         
            //! service-manager needs to have it's own policy for callee::handle::basic_call, since
            //! we can't communicate with blocking to the same queue (with read, who is
            //! going to write? with write, what if the queue is full?)
            struct Policy
            {

               Policy( manager::State& state) : m_state( &state) {}

               Policy( Policy&&) noexcept = default;
               Policy& operator = ( Policy&&) noexcept = default;


               void configure( common::server::Arguments&);

               void reply( common::strong::ipc::id id, common::message::service::call::Reply& message);

               void ack( const common::message::service::call::ACK& ack);

               void transaction(
                     const common::transaction::ID& trid,
                     const common::server::Service& service,
                     const platform::time::unit& timeout,
                     const platform::time::point::type& now);

               common::message::service::Transaction transaction( bool commit);
               void forward( common::service::invoke::Forward&& forward, const common::message::service::call::callee::Request& message);
               void statistics( common::strong::ipc::id, common::message::event::service::Call&);

            private:
               manager::State* m_state;
            };

            using Call = common::server::handle::basic_call< manager::handle::Policy>;

            using dispatch_type = decltype( common::message::dispatch::handler( ipc::device()));

         } // handle
         
         //! @returns all the handlers for service manager
         handle::dispatch_type handler( State& state);

      } // manager
   } // service
} // casual



