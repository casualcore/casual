//!
//! casual_server_context.h
//!
//! Created on: Apr 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_SERVER_CONTEXT_H_
#define CASUAL_SERVER_CONTEXT_H_


#include "common/message.h"
#include "common/ipc.h"
#include "common/queue.h"
#include "common/environment.h"

#include "common/calling_context.h"
#include "common/transaction_context.h"
#include "common/platform.h"
#include "common/log.h"
#include "common/trace.h"


#include <xatmi.h>
#include <xa.h>

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

         struct Service
         {
            Service( const std::string& name, tpservice function)
            : name( name), function( function) {}

            Service( Service&&) = default;


            void call( TPSVCINFO* serviceInformation)
            {
               (*function)( serviceInformation);
            }

            std::string name;
            tpservice function;
            bool startTransaction = false;
            bool active = true;


         };


         struct Arguments
         {
            Arguments() = default;
            Arguments( Arguments&&) = default;

            std::vector< Service> m_services;
            std::function<int( int, char**)> m_server_init = &tpsvrinit;
            std::function<void()> m_server_done = &tpsvrdone;
            int m_argc;
            char** m_argv;
            std::vector< transaction::Resource> resources;
         };

         struct State
         {
            State() = default;

            State( const State&) = delete;
            State& operator = (const State&) = delete;

            typedef std::unordered_map< std::string, Service> service_mapping_type;

            service_mapping_type services;
            common::platform::long_jump_buffer_type long_jump_buffer;

            message::service::Reply reply;
            message::monitor::Notify monitor;

            std::function<void()> m_server_done;

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
            //! Sematics:
            //! - construction
            //! -- send connect to broker - connect server - advertise all services
            //! - dispatch
            //! -- set longjump
            //! -- call user XATMI-service
            //! -- when user calls tpreturn we longjump back
            //! -- send reply to caller
            //! -- send ack to broker
            //! -- send time-stuff to monitor (if active)
            //! -- TODO: transaction stuff
            //! - destruction
            //! -- send disconnect to broker - disconnect server - unadvertise services
            //!
            //! @note it's a template only to be able to unittest it
            //!
            template< typename P>
            struct basic_call
            {
               typedef P policy_type;

               typedef message::service::callee::Call message_type;

               basic_call( basic_call&&) = delete;
               basic_call& operator = ( basic_call&&) = delete;

               basic_call() = delete;
               basic_call( const basic_call&) = delete;
               basic_call& operator = ( basic_call&) = delete;




               //!
               //! Connect @p server to the broker, broker will build a dispatch-table for
               //! coming XATMI-calls
               //!
               template< typename... Args>
               basic_call( server::Arguments& arguments, Args&&... arg) : m_policy( std::forward< Args>( arg)...)
               {
                  m_state.m_server_done = arguments.m_server_done;

                  message::server::Connect message;


                  for( auto&& service : arguments.m_services)
                  {
                     message.services.emplace_back( service.name);
                     m_state.services.emplace( service.name, std::move( service));
                  }

                  std::swap( arguments.resources, transaction::Context::instance().state().resources);

                  //
                  // Connect to casual
                  //
                  auto configuration = m_policy.connect( message);
                  environment::domain::name( configuration.domain);

                  //
                  // Apply the configuration
                  //
                  transaction::Context::instance().apply( configuration);


                  //
                  // Call tpsrvinit
                  //
                  if( ! arguments.m_server_init( arguments.m_argc, arguments.m_argv))
                  {
                     exception::NotReallySureWhatToNameThisException( "service init failed");
                  }

               }


               //!
               //! Sends a message::server::Disconnect to the broker
               //!
               ~basic_call() noexcept
               {

                  try
                  {
                     Trace trace{ "basic_call::~basic_call"};
                     //
                     // Call tpsrvdone
                     //
                     m_state.m_server_done();

                     //
                     // If there are open resources, close'em
                     //
                     transaction::Context::instance().close();

                     m_policy.disconnect();
                  }
                  catch( ...)
                  {
                     error::handler();
                  }

               }


               //!
               //! Handles the actual XATM-call from another process. Dispatch
               //! to the registered function, and "waits" for tpreturn (long-jump)
               //! to send the reply.
               //!
               void dispatch( message_type& message)
               {
                  common::Trace trace{ "basic_call::dispatch"};

                  //
                  // Set starttime. TODO: Should we try to get the startime earlier? Hence, at ipc-queue level?
                  //
                  if( message.service.monitor_queue != 0)
                  {
                     m_state.monitor.start = platform::clock_type::now();
                  }

                  //
                  // Set the call-chain-id for this "chain"
                  //
                  calling::Context::instance().callId( message.callId);

                  //
                  // Do transaction stuff...
                  // - begin transaction if service has "auto-transaction"
                  // - notify TM about potentially resources involved.
                  //
                  m_policy.transaction( message);


                  //
                  // Prepare for tpreturn.
                  //
                  // ATTENTION: no types with destructor should be instantiated between
                  // setjmp and the else clause for 'if( jumpState == 0)'
                  //
                  int jumpState = setjmp( m_state.long_jump_buffer);

                  if( jumpState == 0)
                  {

                     //
                     // No longjmp has been called, this is the first time in this "service call"
                     // Let's call the user service...
                     //

                     //
                     // set the call-correlation
                     //
                     m_state.reply.callDescriptor = message.callDescriptor;

                     auto findIter = m_state.services.find( message.service.name);

                     if( findIter == m_state.services.end())
                     {
                        throw common::exception::xatmi::SystemError( "Service [" + message.service.name + "] not present at server - inconsistency between broker and server");
                     }


                     calling::Context::instance().currentService( message.service.name);

                     TPSVCINFO serviceInformation = transformServiceInformation( message);

                     //
                     // Before we call the user function we have to add the buffer to the "buffer-pool"
                     //
                     buffer::Context::instance().add( std::move( message.buffer));

                     findIter->second.call( &serviceInformation);

                     //
                     // User service returned, not by tpreturn. The standard does not mention this situation, what to do?
                     //
                     throw common::exception::xatmi::service::Error( "Service: " + message.service.name + " did not call tpreturn");

                  }
                  else
                  {

                     //
                     // Do transaction stuff...
                     // - commit/rollback transaction if service has "auto-transaction"
                     //
                     m_policy.transaction( m_state.reply);

                     //
                     // User has called tpreturn.
                     // Send reply to caller. We previously "saved" state when the user called tpreturn.
                     // we now use it
                     //
                     m_policy.reply( message.reply.queue_id, m_state.reply);

                     //
                     // Send ACK to broker
                     //
                     m_policy.ack( message);

                     //
                     // Do some cleanup...
                     //
                     server::Context::instance().finalize();

                     //
                     // Take end time
                     //
                     if( message.service.monitor_queue != 0)
                     {
                        m_state.monitor.end = platform::clock_type::now();
                        m_state.monitor.callId = message.callId;
                        m_state.monitor.service = message.service.name;
                        m_state.monitor.parentService = message.callee;

                        m_policy.statistics( message.service.monitor_queue, m_state.monitor);
                     }
                     calling::Context::instance().currentService( "");
                  }
               }
            private:

               TPSVCINFO transformServiceInformation( message::service::callee::Call& message) const
               {
                  TPSVCINFO result;

                  strncpy( result.name, message.service.name.data(), sizeof( result.name) );
                  result.data = platform::public_buffer( message.buffer.raw());
                  result.len = message.buffer.size();
                  result.cd = message.callDescriptor;
                  result.flags = 0;

                  return result;
               }


               policy_type m_policy;
               server::State& m_state = server::Context::instance().getState();

            };

            namespace policy
            {


               //!
               //! Default policy for basic_call. Only broker and unittest have to define another
               //! policy
               //!
               struct Default
               {
                  template< typename W>
                  struct broker_writer : public W
                  {
                     broker_writer() : W( ipc::getBrokerQueue().id()) {}
                  };


                  message::server::Configuration connect( message::server::Connect& message);

                  void disconnect();

                  void reply( platform::queue_id_type id, message::service::Reply& message);

                  void ack( const message::service::callee::Call& message);

                  void statistics( platform::queue_id_type id, message::monitor::Notify& message);

                  void transaction( const message::service::callee::Call& message);
                  void transaction( const message::service::Reply& message);

               private:
                  typedef queue::blocking::Writer reply_writer;
                  typedef queue::non_blocking::Writer monitor_writer;
                  typedef broker_writer< queue::blocking::Writer> blocking_broker_writer;
                  typedef broker_writer< queue::non_blocking::Writer> non_blocking_broker_writer;

               };



            } // policy

            //!
            //! Handle service calls from other proceses and does a dispatch to
            //! the register XATMI functions.
            //!
            typedef basic_call< policy::Default> Call;

         } // handle
      } // callee
	} // common
} // casual



#endif /* CASUAL_SERVER_CONTEXT_H_ */
