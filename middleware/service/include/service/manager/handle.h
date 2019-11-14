//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "service/manager/state.h"

#include "common/message/dispatch.h"
#include "common/message/server.h"
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
            const common::communication::ipc::Helper& device();
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

            using dispatch_type = decltype( ipc::device().handler());

            void process_exit( const common::process::lifetime::Exit& exit);

            struct Base
            {
               Base( State& state) : m_state( state) {};

            protected:
               State& m_state;
            };


            namespace process
            {
               struct Exit : Base
               {
                  using Base::Base;
                  void operator () ( common::message::event::process::Exit& message);
               };

               namespace prepare
               {
                  struct Shutdown : Base
                  {
                     using Base::Base;
                     void operator () ( common::message::domain::process::prepare::shutdown::Request& message);
                  };
               } // prepare

            } // process


            namespace event
            {
               namespace subscription
               {
                  struct Begin: public Base
                  {
                     using Base::Base;
                     void operator () ( common::message::event::subscription::Begin& message);
                  };

                  struct End: public Base
                  {
                     using Base::Base;
                     void operator () ( common::message::event::subscription::End& message);
                  };

               } // subscription
            } // event

            namespace service
            {
               //! Handles local advertise
               //!  - add  0..* services
               //!  - remove 0..* services
               //!  - replace == remove all services for instance and then add 0..* services
               struct Advertise : Base
               {
                  using Base::Base;
                  void operator () ( common::message::service::Advertise& message);
               };

               //! services (instances rather) that are concurrent, hence does not block
               //! the instance.
               namespace concurrent
               {
                  struct Advertise : Base
                  {
                     using Base::Base;
                     void operator () ( common::message::service::concurrent::Advertise& message);
                  };

                  //! handles metric and "acks" from concurrent servers.
                  struct Metric : Base
                  {
                     using Base::Base;
                     void operator () ( common::message::event::service::Calls& message);
                  };
               } // concurrent

               //! Looks up a service-name
               struct Lookup : Base
               {
                  using Base::Base;
                  void operator () ( common::message::service::lookup::Request& message);
               private:
                  void discover( 
                     common::message::service::lookup::Request&& message, 
                     const std::string& name, 
                     platform::time::point::type now);
               };

               namespace discard
               {
                  //! discards a previously pending lookup
                  struct Lookup : Base
                  {
                     using Base::Base;
                     void operator () ( common::message::service::lookup::discard::Request& message);
                  };

               } // discard

            } // service

            namespace domain
            {
               namespace discover
               {
                  struct Request : Base
                  {
                     using Base::Base;
                     void operator () ( common::message::gateway::domain::discover::Request& message);
                  };

                  struct Reply : Base
                  {
                     using Base::Base;
                     void operator () ( common::message::gateway::domain::discover::accumulated::Reply& message);
                  };

               } // discover
            } // domain

            //! Handles ACK from services.
            //!
            //! if there are pending request for the "acked-service" we
            //! send response directly
            struct ACK : public Base
            {
               using Base::Base;
               void operator () ( const common::message::service::call::ACK& message);
            };


            //! service-manager needs to have it's own policy for callee::handle::basic_call, since
            //! we can't communicate with blocking to the same queue (with read, who is
            //! going to write? with write, what if the queue is full?)
            struct Policy
            {

               Policy( manager::State& state) : m_state( state) {}

               Policy( Policy&&) = default;
               Policy& operator = ( Policy&&) = default;


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
               manager::State& m_state;
            };

            using Call = common::server::handle::basic_call< manager::handle::Policy>;

         } // handle

         handle::dispatch_type handler( State& state);

      } // manager
   } // service
} // casual



