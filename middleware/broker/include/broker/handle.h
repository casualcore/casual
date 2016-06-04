//!
//! casual_broker_transform.h
//!
//! Created on: Jun 15, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BROKER_TRANSFORM_H_
#define CASUAL_BROKER_TRANSFORM_H_


#include "broker/state.h"
#include "broker/broker.h"

#include "common/message/dispatch.h"
#include "common/message/server.h"
#include "common/message/transaction.h"

#include "common/server/handle.h"
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

         void process_exit( const common::process::lifetime::Exit& exit);

         struct Base
         {
            Base( State& state) : m_state( state) {};

         protected:
            State& m_state;
         };



         namespace traffic
         {
            //!
            //! Traffic Connect
            //!
            struct Connect: public Base
            {
               using Base::Base;

               void operator () ( common::message::traffic::monitor::connect::Request& message);
            };

            struct Disconnect: public Base
            {
               typedef common::message::traffic::monitor::Disconnect message_type;

               using Base::Base;

               void operator () ( message_type& message);
            };

         } // monitor



         namespace forward
         {
            struct Connect : Base
            {
               using Base::Base;

               void operator () ( const common::message::forward::connect::Request& message);
            };

         } // forward




         //!
         //! Advertise 0..N services for a server.
         //!
         struct Advertise : public Base
         {
            typedef common::message::service::Advertise message_type;

            using Base::Base;

            void operator () ( message_type& message);
         };


		   //!
         //! Unadvertise 0..N services for a server.
         //!
		   struct Unadvertise : public Base
         {
            typedef common::message::service::Unadvertise message_type;

            using Base::Base;

            void operator () ( message_type& message);
         };


		   namespace service
         {
	         //!
	         //! Looks up a service-name
	         //!
	         struct Lookup : public Base
	         {
	            typedef common::message::service::lookup::Request message_type;

	            using Base::Base;

	            void operator () ( message_type& message);

	         };

         } // service


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


            void connect( std::vector< common::message::Service> services, const std::vector< common::transaction::Resource>& resources);

            void reply( common::platform::ipc::id::type id, common::message::service::call::Reply& message);

            void ack( const common::message::service::call::callee::Request& message);

            void transaction( const common::message::service::call::callee::Request&, const common::server::Service&, const common::platform::time_point&);

            void transaction( const common::message::service::call::Reply& message, int return_state);

            void forward( const common::message::service::call::callee::Request& message, const common::server::State::jump_t& jump);

            void statistics( common::platform::ipc::id::type id, common::message::traffic::Event& event);

         private:

            broker::State& m_state;

         };

         typedef common::server::handle::basic_call< broker::handle::Policy> Call;


		} // handle

      common::message::dispatch::Handler handler( State& state);

      common::message::dispatch::Handler handler_no_services( State& state);



	} // broker
} // casual


#endif /* CASUAL_BROKER_TRANSFORM_H_ */
