//!
//! casual
//!

#ifndef CASUAL_BROKER_TRANSFORM_H_
#define CASUAL_BROKER_TRANSFORM_H_


#include "broker/state.h"
#include "broker/broker.h"

#include "common/message/dispatch.h"
#include "common/message/server.h"
#include "common/message/gateway.h"
#include "common/message/transaction.h"
#include "common/message/event.h"

#include "common/server/handle/call.h"
#include "common/server/context.h"




namespace casual
{

	namespace broker
	{



      namespace ipc
      {
         const common::communication::ipc::Helper& device();
      } // ipc




      namespace handle
      {
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
               using message_type = common::message::event::process::Exit;

               void operator () ( message_type& message);
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
                  typedef common::message::event::subscription::End message_type;

                  using Base::Base;

                  void operator () ( message_type& message);
               };

            } // subscription

         } // event


		   namespace service
         {
            //!
            //! Handles local advertise
            //!  - add  0..* services
            //!  - remove 0..* services
            //!  - replace == remove all services for instance and then add 0..* services
            //!
	         struct Advertise : Base
	         {
	            typedef common::message::service::Advertise message_type;

	            using Base::Base;

	            void operator () ( message_type& message);
	         };


	         //!
	         //! Looks up a service-name
	         //!
	         struct Lookup : Base
	         {
	            using message_type = common::message::service::lookup::Request;

	            using Base::Base;

	            void operator () ( message_type& message);

	         };

         } // service

		   namespace domain
         {
		      //!
            //! Handles remote advertise
            //!  - add  0..* services
            //!  - remove 0..* services
            //!  - replace == remove all services for instance and then add 0..* services
            //!
            struct Advertise : Base
            {
               using message_type = common::message::gateway::domain::Advertise;

               using Base::Base;

               void operator () ( message_type& message);
            };

		      namespace discover
            {
	            struct Request : Base
	            {
	               using message_type = common::message::gateway::domain::discover::Request;
	               using Base::Base;

	               void operator () ( message_type& message);
	            };

               struct Reply : Base
               {
                  using message_type = common::message::gateway::domain::discover::accumulated::Reply;

                  using Base::Base;

                  void operator () ( message_type& message);
               };


            } // discover


         } // domain


         //!
         //! Handles ACK from services.
         //!
         //! if there are pending request for the "acked-service" we
         //! send response directly
         //!
         struct ACK : public Base
         {

            typedef common::message::service::call::ACK message_type;

            using Base::Base;

            void operator () ( message_type& message);
         };





         //!
         //! Broker needs to have it's own policy for callee::handle::basic_call, since
         //! we can't communicate with blocking to the same queue (with read, who is
         //! going to write? with write, what if the queue is full?)
         //!
         struct Policy
         {

            Policy( broker::State& state) : m_state( state) {}

            Policy( Policy&&) = default;
            Policy& operator = ( Policy&&) = default;


            void configure( common::server::Arguments&);

            void reply( common::platform::ipc::id::type id, common::message::service::call::Reply& message);

            void ack();


            void transaction(
                  const common::transaction::ID& trid,
                  const common::server::Service& service,
                  const std::chrono::microseconds& timeout,
                  const common::platform::time::point::type& now);

            common::message::service::Transaction transaction( bool commit);

            void forward( common::service::invoke::Forward&& forward, const common::message::service::call::callee::Request& message);

            void statistics( common::platform::ipc::id::type, common::message::event::service::Call&);

         private:

            broker::State& m_state;

         };

         typedef common::server::handle::basic_call< broker::handle::Policy> Call;


		} // handle

      handle::dispatch_type handler( State& state);


	} // broker
} // casual


#endif /* CASUAL_BROKER_TRANSFORM_H_ */
