//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/server/context.h"

#include "common/buffer/transport.h"
#include "common/execution.h"
#include "common/exception/xatmi.h"
#include "common/log.h"
#include "common/execute.h"

#include "common/flag.h"

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


                  common::service::invoke::Parameter parameter( message::service::call::callee::Request& message);
                  common::service::invoke::Parameter parameter( message::conversation::connect::callee::Request& message);

               } // transform

               namespace complement
               {
                  void reply( common::service::invoke::Result&& result, message::service::call::Reply& reply);
                  void reply( common::service::invoke::Result&& result, message::conversation::caller::Send& reply);

               } // complement

               template< typename P, typename C, typename M>
               void call( P& policy, C& service_context, M&& message, bool send_reply)
               {
                  Trace trace{ "server::handle::service::call"};

                  execution::service::name( message.service.name);
                  execution::service::parent::name( message.parent);

                  common::service::header::fields() = std::move( message.header);



                  //
                  // Make sure we do some cleanup and send ACK to broker.
                  //
                  auto execute_finalize = execute::scope( [&](){
                     policy.ack();
                     server::context().finalize();
                  });


                  //
                  // Prepare reply
                  //
                  auto reply = transform::reply( message);

                  auto execute_reply = execute::scope( [&](){
                     if( send_reply)
                     {
                        //
                        // Send reply to caller.
                        //
                        policy.reply( message.process.queue, reply);
                     }
                  });


                  auto parameter = transform::parameter( message);


                  //
                  // If something goes wrong, make sure to rollback before reply with error.
                  // this will execute before execute_reply
                  //
                  auto execute_error_reply = execute::scope( [&](){
                     reply.transaction = policy.transaction( false);
                  });


                  auto& state = server::Context::instance().state();

                  //
                  // Set start time.
                  //
                  state.event.start = platform::time::clock::type::now();


                  //
                  // Find service
                  //

                  auto found = algorithm::find( state.services, parameter.service.name);

                  if( ! found)
                  {
                     throw common::exception::xatmi::System( "service: " + parameter.service.name + " not present at server - inconsistency between broker and server");
                  }

                  auto& service = found->second;

                  //
                  // Do transaction stuff...
                  // - begin transaction if service has "auto-transaction"
                  // - notify TM about potentially resources involved.
                  // - set 'global' deadline/timeout
                  //
                  policy.transaction( message.trid, service, message.service.timeout, state.event.start);


                  //
                  // Apply pre service buffer manipulation
                  //
                  /*
                  buffer::transport::Context::instance().dispatch(
                        information.data,
                        information.len,
                        information.name,
                        buffer::transport::Lifecycle::pre_service);
                        */

                  //
                  // call the service
                  //
                  try
                  {
                     complement::reply( service( std::move( parameter)), reply);
                  }
                  catch( common::service::invoke::Forward& forward)
                  {
                     policy.forward( std::move( forward), message);

                     execute_reply.release();
                     execute_error_reply.release();

                     return;
                  }



                  // TODO: What are the semantics of 'order' of failure?
                  //       If TM is down, should we send reply to caller?
                  //       If broker is down, should we send reply to caller?


                  //
                  // Apply post service buffer manipulation
                  //
                  /*
                  buffer::transport::Context::instance().dispatch(
                        state.jump.buffer.data,
                        state.jump.buffer.size,
                        message.service.name,
                        buffer::transport::Lifecycle::post_service);
                        */



                  //
                  // Do transaction stuff...
                  // - commit/rollback transaction if service has "auto-transaction"
                  //
                  auto execute_transaction = execute::scope( [&](){
                     reply.transaction = policy.transaction(
                           reply.transaction.state == message::service::Transaction::State::active);
                  });

                  //
                  // Nothing did go wrong
                  //
                  execute_error_reply.release();


                  //
                  // Take end time
                  //
                  auto execute_monitor = execute::scope( [&](){
                     if( ! message.service.event_subscribers.empty())
                     {
                        state.event.end = platform::time::clock::type::now();
                        state.event.execution = message.execution;
                        state.event.service = message.service.name;
                        state.event.parent = message.parent;
                        state.event.trid = reply.transaction.trid;
                        state.event.process = process::handle();

                        for( auto& queue : message.service.event_subscribers)
                        {
                           policy.statistics( queue, state.event);
                        }
                     }
                  });

                  execute_transaction();
                  execute_reply();
                  execute_monitor();
               }

            } // service
         } // handle
      } // server
   } // common


} // casual


