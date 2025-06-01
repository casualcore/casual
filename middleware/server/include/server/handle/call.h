//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "server/argument.h"
#include "server/handle/policy.h"
#include "server/context.h"

#include "service/call/context.h"
#include "service/conversation/context.h"

#include "transaction/context.h"

#include "common/log.h"
#include "common/exception/capture.h"
#include "common/execute.h"
#include "common/move.h"


namespace casual
{
   namespace server::handle
   {
      namespace detail
      {
         namespace transform
         {
            common::message::service::call::Reply reply( const common::message::service::call::callee::Request& message);
            common::message::conversation::callee::Send reply( const common::message::conversation::connect::callee::Request& message);

            server::service::invoke::Parameter parameter( common::message::service::call::callee::Request& message);
            server::service::invoke::Parameter parameter( common::message::conversation::connect::callee::Request& message);

         } // transform

         namespace complement
         {
            void reply( server::service::invoke::Result&& result, common::message::service::call::Reply& reply);
            void reply( server::service::invoke::Result&& result, common::message::conversation::callee::Send& reply);

         } // complement

         template< typename P, typename C, typename M>
         void call( P& policy, C& service_context, M&& message, bool send_reply)
         {
            common::Trace trace{ "server::handle::service::call"};

            auto start = platform::time::clock::type::now();

            common::execution::context::service::set( message.service.name);
            common::execution::context::span::reset();
            common::execution::context::parent::service::set( message.parent.service);
            common::execution::context::parent::span::set( message.parent.span);

            // set deadline (if any) for further service calls downstream
            casual::service::call::context().deadline( start, message.deadline.remaining);
            
            // Prepare current_trid for later;
            decltype( casual::transaction::context().current().trid) current_trid;

            // Prepare reply
            auto reply = transform::reply( message);

            // Make sure we do some cleanup and send ACK to service-manager.
            auto execute_finalize = common::execute::scope( [&]()
            {
               common::message::service::call::ACK ack;

               ack.correlation = message.correlation;
               ack.execution = message.execution;
               ack.metric.span = common::execution::context::get().span;
               ack.metric.execution = message.execution;
               ack.metric.service = message.service.logical_name();
               ack.metric.parent = message.parent;
               ack.metric.process = common::process::handle();
               ack.metric.trid = current_trid;

               ack.metric.start = start;
               ack.metric.end = platform::time::clock::type::now();

               // make sure service-manager "gets back" the pending metric
               ack.metric.pending = message.pending;
               ack.metric.code = reply.code;

               policy.ack( ack);
               server::context().finalize();
            });

            auto execute_reply = common::execute::scope( [&]()
            {
               auto normalize_code = []( auto& reply) -> decltype(auto)
               {
                  using Result = decltype( reply.code.result);
                  if( reply.code.result != Result::ok)
                     return reply;

                  // if transaction state is _not good_ we need to indicate this on the
                  // normal "reply code channel".
                  switch( reply.transaction_state)
                  {
                     // TODO: not totally sure about the exact correlation between 
                     //    transaction.state -> reply.code.result. For now, we use "the worst".
                     using Enum = decltype( reply.transaction_state);
                     case Enum::ok:
                        break;
                     case Enum::rollback:
                     case Enum::timeout:
                     case Enum::error:
                        reply.code.result = Result::service_error;
                        break;
                  }
                  return reply;
               };

               // Send reply to caller.
               if( send_reply)
                  policy.reply( message.process.ipc, normalize_code( reply));
                  
            });


            // If something goes wrong, make sure to rollback before reply with error.
            // this will execute before execute_reply
            auto execute_error_reply = common::execute::scope( [&]()
            {
               reply.transaction_state = policy.transaction( false);
            });

            auto& state = server::Context::instance().state();

            // Find service
            auto found = common::algorithm::find( state.services, message.service.name);

            if( ! found)
               common::code::raise::error( common::code::xatmi::system, message.service.name, " not present at server - inconsistency between service-manager and server");

            auto& service = found->second;

            // Do transaction stuff...
            // - begin transaction if service has "auto-transaction"
            // - notify TM about potentially resources involved.
            policy.transaction( message.trid, service);

            // Need to grab current transactionID before it is committed and gone when
            // execute_finalize executes
            // This is only for metric purposes
            current_trid = casual::transaction::context().current().trid;

            auto parameter = transform::parameter( message);
            
            // transform::parameter( message) may have reserved a descriptor that we need to
            // unreserve! 
            auto execute_unreserve_descriptor = common::execute::scope( [descriptor = parameter.descriptor]()
            {
               if( descriptor)
                  casual::service::conversation::context().descriptors().unreserve( descriptor);
            });


            // call the service
            try
            {
               complement::reply( service( std::move( parameter)), reply);
            }
            catch( casual::server::service::invoke::Forward& forward)
            {
               // TODO make this forward work without a copy of payload...
               policy.forward( std::move( forward), message);
               
               policy.transaction( true);

               execute_reply.release();
               execute_error_reply.release();

               // reply.code.result is used to set outcome in the 'ack'. We might want a _forward_ code?
               reply.code.result = common::code::xatmi::ok;

               return;
            }

            // TODO: What are the semantics of 'order' of failure?
            //       If TM is down, should we send reply to caller?
            //       If broker is down, should we send reply to caller?


            // Do transaction stuff...
            // - commit/rollback transaction if service has "auto-transaction"
            auto execute_transaction = common::execute::scope( [&]()
            {
               reply.transaction_state = policy.transaction( reply.transaction_state == decltype( reply.transaction_state)::ok);
            });

            // Nothing did go wrong
            execute_error_reply.release();

            execute_transaction();
            execute_reply();
            execute_unreserve_descriptor();
         }
      } // detail


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
         using policy_type = P;
         using message_type = common::message::service::call::callee::Request;

         basic_call( basic_call&&) = default;
         basic_call& operator = ( basic_call&&) = default;

         basic_call() = delete;
         basic_call( const basic_call&) = delete;
         basic_call& operator = ( basic_call&) = delete;

         //! Connect @p server to the broker, broker will build a dispatch-table for
         //! coming XATMI-calls
         template< typename... Args>
         basic_call( server::Arguments arguments, Args&&... args)
            : m_policy( std::forward< Args>( args)...)
         {
            common::Trace trace{ "server::handle::basic_call::basic_call"};

            server::context().configure( arguments);

            // Connect to casual
            m_policy.configure( std::move( arguments));
         }


         void operator () ( message_type& message)
         {
            common::Trace trace{ "server::handle::basic_call::operator()"};

            common::log::debug( "message: ", message);

            try
            {
               using Flag = common::message::service::call::request::Flag;
               detail::call( m_policy, casual::service::call::context(), message, ! common::flag::contains( message.flags, Flag::no_reply));
            }
            catch( ...)
            {
               common::log::error( common::exception::capture());
            }
         }

         policy_type m_policy;
         common::move::Active m_active;
      };


      //!
      //! Handle service calls from other proceses and does a dispatch to
      //! the register XATMI functions.
      //!
      using Call = basic_call< policy::call::Default>;

      namespace admin
      {
         using Call = basic_call< policy::call::Admin>;
      } // admin

   } // server::handle
} // casual


