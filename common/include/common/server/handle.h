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

               typedef message::service::callee::Call message_type;

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
                  trace::internal::Scope trace{ "callee::handle::basic_call::basic_call"};

                  auto& state = server::Context::instance().state();

                  state.server_done = arguments.server_done;


                  std::vector< message::Service> services;

                  for( auto&& service : arguments.services)
                  {
                     services.emplace_back( service.name, service.type, service.transaction);
                     state.services.emplace( service.name, std::move( service));
                  }


                  //
                  // Connect to casual
                  //
                  m_policy.connect( std::move( services), arguments.resources);

                  //
                  // Call tpsrvinit
                  //
                  if( arguments.server_init( arguments.argc, arguments.argv) != 0)
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

                        //
                        // If there are open resources, close'em
                        //
                        transaction::Context::instance().close();
                     }
                     catch( ...)
                     {
                        error::handler();
                     }
                  }

               }


               //!
               //! Handles the actual XATM-call from another process. Dispatch
               //! to the registered function, and "waits" for tpreturn (long-jump)
               //! to send the reply.
               //!
               void dispatch( message_type& message)
               {
                  //
                  // Set the call-chain-id for this "chain"
                  //
                  call::Context::instance().execution( message.execution);

                  trace::internal::Scope trace{ "callee::handle::basic_call::dispatch"};

                  auto start = platform::clock_type::now();


                  auto& state = server::Context::instance().state();

                  //
                  // Set starttime. TODO: Should we try to get the startime earlier? Hence, at ipc-queue level?
                  //
                  if( message.service.monitor_queue != 0)
                  {
                     state.monitor.start = start;
                  }


                  //
                  // Prepare for tpreturn.
                  //
                  // ATTENTION: no types with destructor should be instantiated between
                  // setjmp and the else clause for 'if( jumpState == 0)'
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

                     auto findIter = state.services.find( message.service.name);

                     if( findIter == state.services.end())
                     {
                        throw common::exception::xatmi::SystemError( "Service [" + message.service.name + "] not present at server - inconsistency between broker and server");
                     }

                     auto& service = findIter->second;

                     //
                     // Do transaction stuff...
                     // - begin transaction if service has "auto-transaction"
                     // - notify TM about potentially resources involved.
                     // - set 'global' deadline/timeout
                     //
                     m_policy.transaction( message, service, start);


                     call::Context::instance().currentService( message.service.name);

                     //
                     // Also takes care of buffer to pool
                     //
                     TPSVCINFO information = transformServiceInformation( message);


                     //
                     // Apply pre service buffer manipulation
                     //
                     buffer::transport::Context::instance().dispatch(
                           information.data,
                           information.len,
                           information.name,
                           buffer::transport::Lifecycle::pre_service);


                     service.call( &information);

                     //
                     // User service returned, not by tpreturn. The standard does not mention this situation, what to do?
                     //
                     throw common::exception::xatmi::service::Error( "Service: " + message.service.name + " did not call tpreturn");

                  }
                  else
                  {

                     //
                     // Apply post service buffer manipulation
                     //
                     buffer::transport::Context::instance().dispatch(
                           state.jump.state.data,
                           state.jump.state.len,
                           message.service.name,
                           buffer::transport::Lifecycle::post_service);


                     //
                     // Prepare reply
                     //
                     message::service::Reply reply = transform.reply( state.jump.state, message.descriptor);


                     //
                     // Do transaction stuff...
                     // - commit/rollback transaction if service has "auto-transaction"
                     //
                     m_policy.transaction( reply);

                     //
                     // Send ACK to broker
                     //
                     m_policy.ack( message);



                     //
                     // User has called tpreturn.
                     // Send reply to caller. We previously "saved" state when the user called tpreturn.
                     // we now use it
                     //
                     try
                     {
                        m_policy.reply( message.reply.queue, reply);
                     }
                     catch( const exception::queue::Unavailable&)
                     {
                        // TODO: What are the semantics of 'order' of failure?
                        //       If TM is down, should we send reply to caller?
                        //       If broker is down, should we send reply to caller?
                     }


                     //
                     // Take end time
                     //
                     if( message.service.monitor_queue != 0)
                     {
                        state.monitor.end = platform::clock_type::now();
                        state.monitor.callId = message.execution;
                        state.monitor.service = message.service.name;
                        state.monitor.parentService = message.callee;

                        m_policy.statistics( message.service.monitor_queue, state.monitor);
                     }

                     //
                     // Do some cleanup...
                     //
                     server::Context::instance().finalize();

                  }
               }
            private:

               using descriptor_type = platform::descriptor_type;

               struct transform_t
               {
                  message::service::Reply reply( server::State::jump_t::state_t& state, descriptor_type descriptor)
                  {
                     message::service::Reply result;

                     result.value = state.value;
                     result.code = state.code;
                     result.descriptor = descriptor;

                     if( state.data != nullptr)
                     {
                        try
                        {
                           result.buffer = buffer::pool::Holder::instance().extract( state.data);
                        }
                        catch( ...)
                        {
                           result.value = TPESVCERR;
                        }
                     }
                     else
                     {
                        result.buffer = buffer::Payload{ nullptr};
                     }

                     return result;
                  }

               } transform;

               TPSVCINFO transformServiceInformation( message::service::callee::Call& message) const
               {
                  TPSVCINFO result;

                  //
                  // Before we call the user function we have to add the buffer to the "buffer-pool"
                  //
                  //range::copy_max( message.service.name, )
                  strncpy( result.name, message.service.name.c_str(), sizeof( result.name) );
                  result.len = message.buffer.memory.size();
                  result.cd = message.descriptor;
                  result.flags = 0;

                  result.data = buffer::pool::Holder::instance().insert( std::move( message.buffer));

                  return result;
               }

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

                  void reply( platform::queue_id_type id, message::service::Reply& message);

                  void ack( const message::service::callee::Call& message);

                  void statistics( platform::queue_id_type id, message::monitor::Notify& message);

                  void transaction( const message::service::callee::Call& message, const server::Service& service, const platform::time_point& now);
                  void transaction( message::service::Reply& message);

               private:
                  typedef queue::blocking::Writer reply_writer;
                  typedef queue::non_blocking::Writer monitor_writer;
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

                  void reply( platform::queue_id_type id, message::service::Reply& message)
                  {
                     queue_writer writer{ id, m_state};
                     writer( message);

                  }

                  void ack( const message::service::callee::Call& message)
                  {
                     message::service::ACK ack;
                     ack.process = common::process::handle();
                     ack.service = message.service.name;
                     queue_writer brokerWriter( ipc::broker::id(), m_state);
                     brokerWriter( ack);
                  }


                  void statistics( platform::queue_id_type id, message::monitor::Notify& message)
                  {
                     // no-op
                  }

                  void transaction( const message::service::callee::Call&, const server::Service&, const common::platform::time_point&)
                  {
                     // no-op
                  }
                  void transaction( message::service::Reply& message)
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
