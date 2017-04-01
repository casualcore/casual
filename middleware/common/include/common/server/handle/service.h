//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVER_HANDLE_SERVICE_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVER_HANDLE_SERVICE_H_


#include "common/server/context.h"

#include "common/buffer/transport.h"
#include "common/execution.h"

#include "common/flag.h"

#include "common/message/traffic.h"
#include "common/message/service.h"
#include "common/message/conversation.h"


namespace casual
{
   namespace common
   {
      namespace server
      {
         namespace handle
         {
            namespace service
            {
               namespace transform
               {
                  message::service::call::Reply reply( const message::service::call::callee::Request& message);
                  message::conversation::caller::Send reply( const message::conversation::connect::callee::Request& message);


                  TPSVCINFO information( message::service::call::callee::Request& message);
                  TPSVCINFO information( message::conversation::connect::callee::Request& message);

               } // transform

               namespace complement
               {
                  void reply( message::service::call::Reply& reply, const server::state::Jump& jump);
                  void reply( message::conversation::caller::Send& reply, const server::state::Jump& jump);

               } // complement

               template< typename P, typename C, typename M>
               void call( P& policy, C& service_context, M&& message)
               {
                  Trace trace{ "server::handle::service::call"};


                  //
                  // Make sure we do some cleanup...
                  //
                  auto execute_finalize = scope::execute( [](){ server::Context::instance().finalize();});

                  //
                  // Make sure we'll always send ACK to broker
                  //
                  auto execute_ack = scope::execute( [&](){ policy.ack( message);});


                  //
                  // Prepare reply
                  //
                  auto reply = transform::reply( message);

                  using flag_type =  typename decltype( message.flags)::enum_type;

                  //
                  // Make sure we always send reply to caller
                  //
                  auto execute_reply = scope::execute( [&](){
                     if( ! message.flags.exist( flag_type::no_transaction))
                     {
                        //
                        // Send reply to caller.
                        //
                        policy.reply( message.process.queue, reply);
                     }});


                  //
                  // If something goes wrong, make sure to rollback before reply with error.
                  // this will execute before execute_reply
                  //
                  auto execute_error_reply = scope::execute( [&](){ policy.transaction( reply, TPESVCERR); });


                  auto& state = server::Context::instance().state();

                  //
                  // Set start time.
                  //
                  state.traffic.start = platform::time::clock::type::now();


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
                  policy.transaction( message, service, state.traffic.start);


                  //
                  // Also takes care of buffer to pool
                  //
                  TPSVCINFO information = transform::information( message);


                  //
                  // Apply pre service buffer manipulation
                  //
                  buffer::transport::Context::instance().dispatch(
                        information.data,
                        information.len,
                        information.name,
                        buffer::transport::Lifecycle::pre_service);


                  //
                  // Set destination for the coming jump...
                  // we can't wrap the jump in some abstraction since it's
                  // UB (http://en.cppreference.com/w/cpp/utility/program/setjmp)
                  //
                  switch( ::setjmp( state.jump.environment))
                  {
                     case state::Jump::Location::c_no_jump:
                     {
                        //
                        // ATTENTION: no types with destructor should be instantiated
                        // within this scope, since we're jumping back to the switch case, hence
                        // no dtors will run...
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
                           log::category::error << "exception thrown from service: " << message.service.name << std::endl;
                        }

                        //
                        // User service returned, not by tpreturn.
                        //
                        throw common::exception::xatmi::service::Error( "service: " + message.service.name + " did not call tpreturn");
                     }
                     case state::Jump::Location::c_forward:
                     {
                        //
                        // user has called casual_service_forward
                        //

                        if( service_context.pending())
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

                        policy.forward( message, state.jump);

                        //
                        // Forward went ok, make sure we don't do error stuff
                        //
                        execute_reply.release();
                        execute_error_reply.release();

                        return;
                     }
                     default:
                     {
                        throw common::exception::invalid::Semantic{ "unexpected value from setjmp"};
                     }
                     case state::Jump::Location::c_return:
                     {
                        log::debug << "user called tpreturn\n";

                        //
                        // we continue below...
                        //

                        break;
                     }
                  }

                  // TODO: What are the semantics of 'order' of failure?
                  //       If TM is down, should we send reply to caller?
                  //       If broker is down, should we send reply to caller?


                  //
                  // Apply post service buffer manipulation
                  //
                  buffer::transport::Context::instance().dispatch(
                        state.jump.buffer.data,
                        state.jump.buffer.size,
                        message.service.name,
                        buffer::transport::Lifecycle::post_service);

                  //
                  // Modify reply with stuff from tpreturn...
                  //
                  complement::reply( reply, state.jump);



                  //
                  // Do transaction stuff...
                  // - commit/rollback transaction if service has "auto-transaction"
                  //
                  auto execute_transaction = scope::execute( [&](){ policy.transaction( reply, state.jump.state.value); });

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
                        state.traffic.end = platform::time::clock::type::now();
                        state.traffic.execution = message.execution;
                        state.traffic.service = message.service.name;
                        state.traffic.parent = message.parent;
                        state.traffic.process = process::handle();

                        for( auto& queue : message.service.traffic_monitors)
                        {
                           policy.statistics( queue, state.traffic);
                        }
                     }
                  });

                  execute_ack();
                  execute_transaction();
                  execute_reply();
                  execute_monitor();
               }

            } // service
         } // handle
      } // server
   } // common


} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVER_HANDLE_SERVICE_H_
