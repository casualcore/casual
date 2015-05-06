//!
//! handle.h
//!
//! Created on: Dec 13, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_SESRVER_HANDLE_H_
#define CASUAL_COMMON_SESRVER_HANDLE_H_


#include "common/server/argument.h"
#include "common/server/context.h"

#include "common/internal/trace.h"


#include "common/transaction/context.h"
#include "common/queue.h"

#include "common/call/context.h"
#include "common/buffer/pool.h"
#include "common/buffer/transport.h"

#include "common/flag.h"

namespace casual
{
   namespace common
   {
      namespace server
      {

         message::server::connect::Reply connect( std::vector< message::Service> services);

         message::server::connect::Reply connect( std::vector< message::Service> services, const std::vector< transaction::Resource>& resources);


         template< typename P>
         struct Connect
         {
            using policy_type = P;

            template< typename... Args>
            message::server::connect::Reply operator () ( std::vector< message::Service> services, Args&& ...args)
            {
               using queue_writer = common::queue::blocking::basic_writer< policy_type>;
               using queue_reader = common::queue::blocking::basic_reader< policy_type>;

               message::server::connect::Request message;

               message.process = common::process::handle();
               message.path = common::process::path();
               message.services = std::move( services);

               queue_writer broker( ipc::broker::id(), args...);
               broker( message);

               //
               // Wait for configuration reply
               //
               queue_reader reader( ipc::receive::queue(), args...);
               message::server::connect::Reply reply;
               reader( reply);

               return reply;
            }
         };



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
               basic_call( server::Arguments arguments, Args&&... args) : m_policy( std::forward< Args>( args)...)
               {
                  trace::internal::Scope trace{ "server::handle::basic_call::basic_call"};

                  auto& state = server::Context::instance().state();

                  state.server_done = arguments.server_done;


                  std::vector< message::Service> services;

                  for( auto&& service : arguments.services)
                  {
                     services.emplace_back( service.name, service.type, service::transaction::mode( service.transaction));
                     state.services.emplace( service.name, std::move( service));
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
                  call::Context::instance().execution( message.execution);

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
                  scope::Execute execute_finalize{ [](){ server::Context::instance().finalize();}};

                  //
                  // Make sure we'll always send ACK to broker
                  //
                  scope::Execute execute_ack{ [&](){ m_policy.ack( message); } };


                  //
                  // Prepare reply
                  //
                  auto reply = transform::reply( message);


                  //
                  // Make sure we always send reply to caller
                  //
                  scope::Execute execute_reply{ [&](){
                     if( ! flag< TPNOREPLY>( message.flags))
                     {
                        //
                        // Send reply to caller.
                        //
                        m_policy.reply( message.reply.queue, reply);
                     } } };



                  auto& state = server::Context::instance().state();

                  //
                  // Set start time.
                  //
                  state.monitor.start = platform::clock_type::now();


                  //
                  // set the call-correlation
                  //

                  auto found = state.services.find( message.service.name);

                  if( found == state.services.end())
                  {
                     throw common::exception::xatmi::SystemError( "service: " + message.service.name + " not present at server - inconsistency between broker and server");
                  }

                  auto& service = found->second;

                  call::Context::instance().service( service.name);


                  //
                  // Do transaction stuff...
                  // - begin transaction if service has "auto-transaction"
                  // - notify TM about potentially resources involved.
                  // - set 'global' deadline/timeout
                  //
                  m_policy.transaction( message, service, state.monitor.start);


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
                  // ATTENTION: no types with destructor should be instantiated
                  // within the if( ! jumped) clause
                  //
                  auto jumped = setjmp( state.long_jump_buffer) != 0;

                  if( ! jumped)
                  {

                     //
                     // No longjmp has been called, this is the first time in this "service call"
                     // Let's call the user service...
                     //

                     try
                     {
                        service.call( &information);
                     }
                     catch( ...)
                     {
                        error::handler();
                        log::error << "exception thrown from service: " << message.service.name << std::endl;
                     }

                     //
                     // User service returned, not by tpreturn.
                     //
                     m_policy.transaction( reply, TPESVCERR);

                     throw common::exception::xatmi::service::Error( "service: " + message.service.name + " did not call tpreturn");
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
                        state.jump.state.data,
                        state.jump.state.len,
                        message.service.name,
                        buffer::transport::Lifecycle::post_service);


                  //
                  // Modify reply
                  //
                  transform::reply( reply, state.jump.state);



                  //
                  // Do transaction stuff...
                  // - commit/rollback transaction if service has "auto-transaction"
                  //
                  scope::Execute execute_transaction{ [&](){ m_policy.transaction( reply, state.jump.state.value); } };


                  //
                  // Take end time
                  //
                  scope::Execute execute_monitor{ [&](){
                     if( ! message.service.traffic_monitors.empty())
                     {
                        state.monitor.end = platform::clock_type::now();
                        state.monitor.callId = message.execution;
                        state.monitor.service = message.service.name;
                        state.monitor.parentService = message.caller;
                        state.monitor.pid = process::handle().pid;

                        for( auto& queue : message.service.traffic_monitors)
                        {
                           m_policy.statistics( queue, state.monitor);
                        }
                     }
                  }};

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

                  static void reply( message::service::call::Reply& reply, const server::State::jump_t::state_t& state)
                  {
                     reply.code = state.code;
                     reply.error = 0;

                     if( state.data != nullptr)
                     {
                        try
                        {
                           reply.buffer = buffer::pool::Holder::instance().release( state.data, state.len);
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
                        TPSVCINFO result;

                        //
                        // Before we call the user function we have to add the buffer to the "buffer-pool"
                        //
                        //range::copy_max( message.service.name, )
                        strncpy( result.name, message.service.name.c_str(), sizeof( result.name) );
                        result.len = message.buffer.memory.size();
                        result.cd = message.descriptor;
                        result.flags = message.flags;

                        result.data = buffer::pool::Holder::instance().insert( std::move( message.buffer));

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
                  template< typename W>
                  struct broker_writer : public W
                  {
                     broker_writer() : W( ipc::broker::id()) {}
                  };


                  void connect( std::vector< message::Service> services, const std::vector< transaction::Resource>& resources);

                  void reply( platform::queue_id_type id, message::service::call::Reply& message);

                  void ack( const message::service::call::callee::Request& message);

                  void statistics( platform::queue_id_type id, message::traffic::monitor::Notify& message);

                  void transaction( const message::service::call::callee::Request& message, const server::Service& service, const platform::time_point& now);
                  void transaction( message::service::call::Reply& message, int return_state);

               private:
                  typedef queue::blocking::Writer reply_writer;
                  typedef broker_writer< queue::blocking::Writer> blocking_broker_writer;
                  typedef broker_writer< queue::non_blocking::Writer> non_blocking_broker_writer;

               };


               template< typename S, typename P = common::queue::policy::RemoveOnTerminate< S> >
               struct Admin
               {
                  using state_type = S;
                  using policy_type = P;


                  using queue_writer = common::queue::blocking::basic_writer< policy_type>;
                  using queue_reader = common::queue::blocking::basic_reader< policy_type>;


                  Admin( state_type& state) : m_state( state) {}


                  void connect( std::vector< message::Service> services, const std::vector< transaction::Resource>& resources)
                  {
                     Connect< policy_type> connect;

                     connect( std::move( services), m_state);
                  }

                  void reply( platform::queue_id_type id, message::service::call::Reply& message)
                  {
                     queue_writer writer{ id, m_state};
                     writer( message);

                  }

                  void ack( const message::service::call::callee::Request& message)
                  {
                     message::service::call::ACK ack;
                     ack.process = common::process::handle();
                     ack.service = message.service.name;
                     queue_writer brokerWriter( ipc::broker::id(), m_state);
                     brokerWriter( ack);
                  }


                  void statistics( platform::queue_id_type id, message::traffic::monitor::Notify& message)
                  {
                     // no-op
                  }

                  void transaction( const message::service::call::callee::Request&, const server::Service&, const common::platform::time_point&)
                  {
                     // no-op
                  }
                  void transaction( message::service::call::Reply& message, int return_state)
                  {
                     // no-op
                  }


                  state_type& m_state;
               };


            } // policy

            //!
            //! Handle service calls from other proceses and does a dispatch to
            //! the register XATMI functions.
            //!
            typedef basic_call< policy::Default> Call;

            template< typename S>
            using basic_admin_call = basic_call< policy::Admin< S>>;


         } // handle

      } // server
   } // common
} // casual

#endif // HANDLE_H_
