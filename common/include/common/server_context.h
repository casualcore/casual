//!
//! casual_server_context.h
//!
//! Created on: Apr 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_SERVER_CONTEXT_H_
#define CASUAL_SERVER_CONTEXT_H_

#include "common/service_context.h"
#include "common/message.h"
#include "common/ipc.h"
#include "common/queue.h"
#include "common/transform.h"
#include "common/types.h"

#include "common/calling_context.h"

#include "utility/platform.h"
#include "utility/logger.h"


#include "xatmi.h"

//
// std
//
#include <unordered_map>


namespace casual
{
   namespace common
   {
      namespace server
      {
         struct State
         {
            State() = default;

            State( const State&) = delete;
            State& operator = (const State&) = delete;

            typedef std::unordered_map< std::string, service::Context> service_mapping_type;

            service_mapping_type services;
            utility::platform::long_jump_buffer_type long_jump_buffer;

            message::service::Reply reply;
            message::monitor::Notify monitor;

         };

         class Context
         {
         public:


            static Context& instance();

            Context( const Context&) = delete;


            //!
            //! Being called from tpreturn
            //!
            void longJumpReturn( int rval, long rcode, char* data, long len, long flags);

            //!
            //! Being called from tpadvertise
            //!
            void advertiseService( const std::string& name, tpservice function);

            //!
            //! Being called from tpunadvertise
            //!
            void unadvertiseService( const std::string& name);

            //!
            //! Share state with calle::handle::basic_call
            //!
            State& getState();

            void finalize();


         private:

            Context();

            State m_state;
         };

      } // server

      namespace callee
      {
         namespace handle
         {

            //!
            //! Handles XATMI-calls
            //!
            template< typename P>
            struct basic_call
            {
               typedef P queue_policy;

               typedef message::service::callee::Call message_type;

               basic_call() = delete;
               basic_call( const basic_call&) = delete;

               //!
               //! Advertise @p services to the broker build a dispatch-table for
               //! coming XATMI-calls
               //!
               basic_call( std::vector< service::Context>& services)
               {
                  message::service::Advertise message;
                  message.serverId.queue_key = queue_policy::receiveKey();


                  for( auto&& service : services)
                  {
                     message.services.emplace_back( service.m_name);
                     m_state.services.emplace( service.m_name, std::move( service));

                  }

                  //
                  // Let the broker know about us, and our services...
                  //
                  typename queue_policy::blocking_broker_writer brokerWriter;
                  brokerWriter( message);
               }

               //!
               //! Sends a message::server::Disconnect to the broker
               //!
               ~basic_call() noexcept
               {
                  message::server::Disconnect message;

                  //
                  // we can't block here...
                  // TODO: exception safety
                  //
                  typename queue_policy::non_blocking_broker_writer brokerWriter;
                  brokerWriter( message);

               }


               //!
               //! Handles the actual XATM-call from another process. Dispatch
               //! to the registered function, and "waits" for tpreturn (long-jump)
               //! to send the reply.
               //!
               void dispatch( message_type& message)
               {

                  server::State& state = server::Context::instance().getState();

                  //
                  // Set starttime. TODO: Should we try to get the startime earlier? Hence, at ipc-queue level?
                  //
                  if( message.service.monitor_queue != 0)
                  {
                     state.monitor.start = common::clock_type::now();
                  }


                  //
                  // Set the call-chain-id for this "chain"
                  //
                  calling::Context::instance().setCallId( message.callId);


                  //
                  // Prepare for tpreturn.
                  //
                  int jumpState = setjmp( state.long_jump_buffer);

                  if( jumpState == 0)
                  {
                     //
                     // No longjmp has been called, this is the first time in this "service call"
                     // Let's call the user service...
                     //

                     //
                     // set the call-correlation
                     //
                     state.reply.callDescriptor = message.callDescriptor;

                     auto findIter = state.services.find( message.service.name);

                     if( findIter == state.services.end())
                     {
                        throw utility::exception::xatmi::SystemError( "Service [" + message.service.name + "] not present at server - inconsistency between broker and server");
                     }


                     calling::Context::instance().setCurrentService( message.service.name);

                     TPSVCINFO serviceInformation = transform::ServiceInformation()( message);

                     //
                     // Before we call the user function we have to add the buffer to the "buffer-pool"
                     //
                     buffer::Context::instance().add( std::move( message.buffer));

                     findIter->second.call( &serviceInformation);

                     //
                     // User service returned, not by tpreturn. The standard does not mention this situation, what to do?
                     //
                     throw utility::exception::xatmi::service::Error( "Service: " + message.service.name + " did not call tpreturn");

                  }
                  else
                  {


                     //
                     // User has called tpreturn.
                     // Send reply to caller. We previously "saved" state when the user called tpreturn.
                     // we now use it
                     //
                     typename queue_policy::blocking_send_writer replyWriter( message.reply.queue_key);
                     replyWriter( state.reply);

                     //
                     // Send ACK to broker
                     //
                     message::service::ACK ack;
                     ack.server.queue_key = queue_policy::receiveKey();
                     ack.service = message.service.name;

                     typename queue_policy::blocking_broker_writer brokerWriter;
                     brokerWriter( ack);

                     //
                     // Do some cleanup...
                     //
                     server::Context::instance().finalize();

                     //
                     // Take end time
                     //
                     if( message.service.monitor_queue != 0)
                     {
                        state.monitor.end = common::clock_type::now();
                        state.monitor.callId = message.callId;
                        state.monitor.service = message.service.name;
                        state.monitor.parentService = message.callee;

                        ipc::send::Queue queue( message.service.monitor_queue);
                        queue::non_blocking::Writer writer( queue);

                        if( ! writer( state.monitor))
                        {
                           utility::logger::warning << "could not write to monitor queue";
                        }

                     }




                  }

               }
            private:

               server::State& m_state = server::Context::instance().getState();

            };

            namespace basic
            {


               //!
               //! Default policy for basic_call. Only broker and unittest have to define another
               //! policy
               //!
               struct queue_policy
               {
                  template< typename W>
                  struct broker_writer : public W
                  {
                     broker_writer() : W( ipc::getBrokerQueue()) {}
                  };

                  typedef broker_writer< queue::blocking::Writer> blocking_broker_writer;
                  typedef broker_writer< queue::non_blocking::Writer> non_blocking_broker_writer;
                  typedef queue::basic_queue< ipc::send::Queue, queue::blocking::Writer> blocking_send_writer;

                  static ipc::receive::Queue::queue_key_type receiveKey() { return ipc::getReceiveQueue().getKey(); }

               };
            } // basic

            //!
            //! Handle service calls from other proceses and does a dispatch to
            //! the register XATMI functions.
            //!
            typedef basic_call< basic::queue_policy> Call;

         } // handle
      } // callee
	} // common
} // casual



#endif /* CASUAL_SERVER_CONTEXT_H_ */
