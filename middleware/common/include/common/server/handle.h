//!
//! casual
//!

#ifndef CASUAL_COMMON_SESRVER_HANDLE_H_
#define CASUAL_COMMON_SESRVER_HANDLE_H_


#include "common/server/argument.h"
#include "common/server/context.h"

#include "common/internal/trace.h"


#include "common/transaction/context.h"

#include "common/service/call/context.h"
#include "common/buffer/pool.h"
#include "common/buffer/transport.h"

#include "common/message/service.h"
#include "common/message/server.h"
#include "common/message/handle.h"

#include "common/communication/ipc.h"

#include "common/flag.h"

namespace casual
{
   namespace common
   {
      namespace server
      {


         namespace handle
         {

            //!
            //! Handles XATMI-calls
            //!
            //! Semantics:
            //! - construction
            //! -- send connect to broker - connect server - advertise all services
            //! - dispatch
            //! -- set longjump
            //! -- call user XATMI-service
            //! -- when user calls tpreturn we longjump back
            //! -- send reply to caller
            //! -- send ack to broker
            //! -- send time-stuff to monitor (if active)
            //! -- transaction stuff
            //! - destruction
            //! -- send disconnect to broker - disconnect server - unadvertise services
            //!
            //! @note it's a template so we can use the same implementation in casual-broker and
            //!    others that need's another policy (otherwise it would send messages to it self, and so on)
            //!
            template< typename P>
            struct basic_call
            {
               typedef P policy_type;

               typedef message::service::call::callee::Request message_type;

               basic_call( basic_call&&) = default;
               basic_call& operator = ( basic_call&&) = default;

               basic_call() = delete;
               basic_call( const basic_call&) = delete;
               basic_call& operator = ( basic_call&) = delete;




               //!
               //! Connect @p server to the broker, broker will build a dispatch-table for
               //! coming XATMI-calls
               //!
               template< typename... Args>
               basic_call( server::Arguments arguments, Args&&... args)
                  : m_policy( std::forward< Args>( args)...)
               {
                  trace::internal::Scope trace{ "server::handle::basic_call::basic_call"};

                  auto& state = server::Context::instance().state();

                  state.server_done = arguments.server_done;


                  std::vector< message::service::advertise::Service> services;

                  for( auto& service : arguments.services)
                  {
                     auto name = service.origin;
                     services.emplace_back( name, service.type, service.transaction);

                     state.physical_services.push_back( std::move( service));
                     state.services.emplace( name, state.physical_services.back());
                  }


                  //
                  // Connect to casual
                  //
                  m_policy.connect( std::move( services), arguments.resources);

                  //
                  // Call tpsrvinit
                  //
                  if( arguments.server_init( arguments.argc, arguments.argv) == -1)
                  {
                     throw exception::NotReallySureWhatToNameThisException( "service init failed");
                  }
               }


               //!
               //! Sends a message::server::Disconnect to the broker
               //!
               ~basic_call() noexcept
               {
                  if( ! m_moved)
                  {
                     try
                     {
                        auto& state = server::Context::instance().state();

                        //
                        // Call tpsrvdone
                        //
                        state.server_done();
                     }
                     catch( ...)
                     {
                        error::handler();
                     }
                  }

               }

               void operator () ( message_type& message)
               {
                  //
                  // Set the call-chain-id for this "chain"
                  //

                  trace::internal::Scope trace{ "server::handle::basic_call::operator()"};

                  log::internal::debug << "message: " << message << '\n';

                  try
                  {
                     dispatch( message);
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

                  trace::internal::Scope trace{ "server::handle::basic_call::dispatch"};


                  //
                  // Make sure we do some cleanup...
                  //
                  auto execute_finalize = scope::execute( [](){ server::Context::instance().finalize();});

                  //
                  // Make sure we'll always send ACK to broker
                  //
                  auto execute_ack = scope::execute( [&](){ m_policy.ack( message);});


                  //
                  // Prepare reply
                  //
                  auto reply = transform::reply( message);


                  //
                  // Make sure we always send reply to caller
                  //
                  auto execute_reply = scope::execute( [&](){
                     if( ! flag< TPNOREPLY>( message.flags))
                     {
                        //
                        // Send reply to caller.
                        //
                        m_policy.reply( message.process.queue, reply);
                     }});


                  //
                  // If something goes wrong, make sure to rollback before reply with error.
                  // this will execute before execute_reply
                  //
                  auto execute_error_reply = scope::execute( [&](){ m_policy.transaction( reply, TPESVCERR); });


                  auto& state = server::Context::instance().state();

                  //
                  // Set start time.
                  //
                  state.traffic.start = platform::clock_type::now();


                  //
                  // set the call-correlation
                  //

                  auto found = range::find( state.services, message.service.name);

                  if( ! found)
                  {
                     throw common::exception::xatmi::System( "service: " + message.service.name + " not present at server - inconsistency between broker and server");
                  }

                  auto& service = found->second;

                  execution::service::name( message.service.name);
                  execution::service::parent::name( message.parent);

                  //
                  // set header
                  //
                  common::service::header::fields( std::move( message.header));




                  //
                  // Do transaction stuff...
                  // - begin transaction if service has "auto-transaction"
                  // - notify TM about potentially resources involved.
                  // - set 'global' deadline/timeout
                  //
                  m_policy.transaction( message, service, state.traffic.start);


                  //
                  // Also takes care of buffer to pool
                  //
                  TPSVCINFO information = transform::service::information( message);


                  //
                  // Apply pre service buffer manipulation
                  //
                  buffer::transport::Context::instance().dispatch(
                        information.data,
                        information.len,
                        information.name,
                        buffer::transport::Lifecycle::pre_service);


                  //
                  // Prepare for tpreturn.
                  //
                  auto from = setjmp( state.long_jump_buffer);

                  if( from == State::jump_t::From::c_no_jump)
                  {
                     //
                     // ATTENTION: no types with destructor should be instantiated
                     // within this scope
                     //

                     //
                     // No longjmp has been called, this is the first time in this "service call"
                     // Let's call the user service...
                     //

                     try
                     {
                        service.get().call( &information);
                     }
                     catch( ...)
                     {
                        error::handler();
                        log::error << "exception thrown from service: " << message.service.name << std::endl;
                     }

                     //
                     // User service returned, not by tpreturn.
                     //
                     throw common::exception::xatmi::service::Error( "service: " + message.service.name + " did not call tpreturn");
                  }
                  else if( from == State::jump_t::From::c_forward)
                  {
                     //
                     // user has called casual_service_forward
                     //

                     if( common::service::call::Context::instance().pending())
                     {
                        //
                        // We can't do a forward, user has pending stuff in flight
                        //
                        throw common::exception::xatmi::service::Error( "service: " + message.service.name + " tried to forward with pending actions");
                     }

                     //
                     // Check if service is present at this instance
                     //
                     if( range::find( state.services, state.jump.forward.service))
                     {
                        throw common::exception::xatmi::service::Error( "not implemented to forward to a service in the same instance");
                     }

                     m_policy.forward( message, state.jump);

                     //
                     // Forward went ok, make sure we don't do error stuff
                     //
                     execute_reply.release();
                     execute_error_reply.release();

                     return;
                  }

                  //
                  // User has called tpreturn
                  //



                  // TODO: What are the semantics of 'order' of failure?
                  //       If TM is down, should we send reply to caller?
                  //       If broker is down, should we send reply to caller?


                  //
                  // Apply post service buffer manipulation
                  //
                  buffer::transport::Context::instance().dispatch(
                        state.jump.buffer.data,
                        state.jump.buffer.len,
                        message.service.name,
                        buffer::transport::Lifecycle::post_service);

                  //
                  // Modify reply
                  //
                  transform::reply( reply, state.jump);



                  //
                  // Do transaction stuff...
                  // - commit/rollback transaction if service has "auto-transaction"
                  //
                  auto execute_transaction = scope::execute( [&](){ m_policy.transaction( reply, state.jump.state.value); });

                  //
                  // Nothing did go wrong
                  //
                  execute_error_reply.release();


                  //
                  // Take end time
                  //
                  auto execute_monitor = scope::execute( [&](){
                     if( ! message.service.traffic_monitors.empty())
                     {
                        state.traffic.end = platform::clock_type::now();
                        state.traffic.execution = message.execution;
                        state.traffic.service = message.service.name;
                        state.traffic.parent = message.parent;
                        state.traffic.process = process::handle();

                        for( auto& queue : message.service.traffic_monitors)
                        {
                           m_policy.statistics( queue, state.traffic);
                        }
                     }
                  });



                  execute_ack();
                  execute_transaction();
                  execute_reply();
                  execute_monitor();
               }

            private:


               using descriptor_type = platform::descriptor_type;

               struct transform
               {

                  static message::service::call::Reply reply( const message::service::call::callee::Request& message)
                  {
                     message::service::call::Reply result;

                     result.correlation = message.correlation;
                     result.descriptor = message.descriptor;
                     result.buffer = buffer::Payload{ nullptr};
                     result.error = TPESVCERR;

                     return result;
                  }

                  static void reply( message::service::call::Reply& reply, const server::State::jump_t& jump)
                  {
                     reply.code = jump.state.code;
                     reply.error = 0;

                     if( jump.buffer.data != nullptr)
                     {
                        try
                        {
                           reply.buffer = buffer::pool::Holder::instance().release( jump.buffer.data, jump.buffer.len);
                        }
                        catch( ...)
                        {
                           error::handler();
                           reply.error = TPESVCERR;
                        }
                     }
                     else
                     {
                        reply.buffer = buffer::Payload{ nullptr};
                     }
                  }

                  struct service
                  {
                     static TPSVCINFO information( message::service::call::callee::Request& message)
                     {
                        trace::internal::Scope trace{ "server::handle::basic_call::service::information"};

                        TPSVCINFO result;

                        //
                        // Before we call the user function we have to add the buffer to the "buffer-pool"
                        //
                        //range::copy_max( message.service.name, )
                        strncpy( result.name, message.service.name.c_str(), sizeof( result.name) );
                        result.len = message.buffer.memory.size();
                        result.cd = message.descriptor;
                        result.flags = message.flags;

                        //
                        // This is the only place where we use adopt
                        //
                        result.data = buffer::pool::Holder::instance().adopt( std::move( message.buffer));

                        return result;
                     }
                  };

               };

               policy_type m_policy;
               move::Moved m_moved;
            };

            namespace policy
            {


               //!
               //! Default policy for basic_call. Only broker and unittest have to define another
               //! policy
               //!
               struct Default
               {
                  void connect( std::vector< message::service::advertise::Service> services, const std::vector< transaction::Resource>& resources);

                  void reply( platform::ipc::id::type id, message::service::call::Reply& message);

                  void ack( const message::service::call::callee::Request& message);

                  void statistics( platform::ipc::id::type id, message::traffic::Event& event);

                  void transaction( const message::service::call::callee::Request& message, const server::Service& service, const platform::time_point& now);
                  void transaction( message::service::call::Reply& message, int return_state);

                  void forward( const message::service::call::callee::Request& message, const State::jump_t& jump);
               };


               struct Admin
               {
                  Admin( communication::error::type handler);


                  void connect( std::vector< message::service::advertise::Service> services, const std::vector< transaction::Resource>& resources);

                  void reply( platform::ipc::id::type id, message::service::call::Reply& message);

                  void ack( const message::service::call::callee::Request& message);


                  void statistics( platform::ipc::id::type id, message::traffic::Event&);

                  void transaction( const message::service::call::callee::Request&, const server::Service&, const common::platform::time_point&);

                  void transaction( message::service::call::Reply& message, int return_state);

                  void forward( const common::message::service::call::callee::Request& message, const common::server::State::jump_t& jump);

               private:
                  communication::error::type m_error_handler;
               };


            } // policy

            //!
            //! Handle service calls from other proceses and does a dispatch to
            //! the register XATMI functions.
            //!
            typedef basic_call< policy::Default> Call;

            using basic_admin_call = basic_call< policy::Admin>;


         } // handle

      } // server
   } // common
} // casual

#endif // HANDLE_H_
