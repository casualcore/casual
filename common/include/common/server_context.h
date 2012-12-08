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

#include "utility/platform.h"


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
         };

         class Context
         {
         public:


            static Context& instance();

            Context( const Context&) = delete;

            //void add( service::Context&& context);



            void longJumpReturn( int rval, long rcode, char* data, long len, long flags);

            void advertiseService( const std::string& name, tpservice function);

            void unadvertiseService( const std::string& name);

            //!
            //!
            //!
            State& getState();

            void finalize();


         private:

            Context();

            void connect();

            void disconnect();


            State m_state;
         };

      } // server

      namespace callee
      {
         namespace handle
         {

            //!
            //!
            //!
            template< typename P>
            struct basic_call
            {
               typedef P queue_policy;

               typedef message::service::callee::Call message_type;

               basic_call() = delete;
               basic_call( const basic_call&) = delete;

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


               void dispatch( message_type& message)
               {

                  server::State& state = server::Context::instance().getState();

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

                  }

               }
            private:

               server::State& m_state = server::Context::instance().getState();

            };

            namespace basic
            {


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
            }

            typedef basic_call< basic::queue_policy> Call;


         } // handle


      } // callee


	} // common
} // casual



#endif /* CASUAL_SERVER_CONTEXT_H_ */
