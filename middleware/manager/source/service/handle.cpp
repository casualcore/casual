//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/manager/service/handle.h"

#include "common/log.h"
#include "common/message/service.h"
#include "common/execution/context.h"

namespace casual
{
   using namespace common;

   namespace manager::service::handle
   {
      namespace local
      {
         namespace
         {
            auto transform_code( invoke::result::Code code)
            {
               return common::service::Code{
                  .result = code.result == decltype( code.result)::success ? common::code::xatmi::ok : common::code::xatmi::service_fail,
                  .user = code.user
               };
            
            }
         } // <unnamed>
      } // local

      namespace detail
      {
         void set_execution_context( const common::message::service::call::callee::Request& message)
         {
            Trace trace{ "manager::service::handle::detail::set_execution_context"};
            
            execution::context::span::reset();
            execution::context::service::set( message.service.name);
            execution::context::parent::service::set( message.parent.service);
            execution::context::parent::span::set( message.parent.span);
         }

         void finalize()
         {
            Trace trace{ "manager::service::handle::detail::finalize"};

            execution::context::reset();
         }

         bool caller_wants_reply( common::message::service::call::request::Flag flags)
         {
            return ! flag::contains( flags, common::message::service::call::request::Flag::no_reply);
         }

         void send_reply( const common::strong::ipc::id& ipc, const common::message::service::call::Reply& reply)
         {
            Trace trace{ "manager::service::handle::detail::send_reply"};

            if( ! common::communication::device::blocking::optional::send( ipc, reply))
               common::log::error( common::code::casual::communication_unavailable, " failed to send service-call-reply to: ", ipc, " - action: ignore");
         }


         common::message::service::call::ACK prepare_ack( const common::message::service::call::callee::Request& message, common::chronology::time_point start)
         {
            Trace trace{ "manager::service::handle::detail::prepare_ack"};

            common::message::service::call::ACK result;
            result.correlation = message.correlation;
            result.execution = message.execution;
            
            result.metric.start = start;
            result.metric.service = message.service.name;
            result.metric.process = process::handle();
            result.metric.correlation = message.correlation;
            result.metric.execution = message.execution;
            result.metric.span = execution::context::get().span;
            result.metric.parent = message.parent;
            result.metric.pending = message.pending;

            return result;
         }

         common::message::service::call::Reply prepare_reply( const common::message::service::call::callee::Request& message)
         {
            Trace trace{ "manager::service::handle::detail::prepare_reply"};

            auto result = common::message::reverse::type( message);
            result.code.result = common::code::xatmi::service_error;

            return result;
         }

         void complement_reply( invoke::Result&& result, common::message::service::call::Reply& reply)
         {
            Trace trace{ "manager::service::handle::detail::complement_reply"};

            reply.code = local::transform_code( result.code);
            reply.buffer = std::move( result.payload);
         }

         namespace transform
         {
            invoke::Parameter parameter( common::message::service::call::callee::Request&& message)
            {
               Trace trace{ "manager::service::handle::detail::transform::parameter"};

               common::log::debug( "message: ", message);

               using parameter_flag = manager::service::invoke::Parameter::Flag;

               return invoke::Parameter{ 
                  .flags = flag::convert( parameter_flag::no_reply, message.flags),
                  .service = std::move( message.service.name),
                  .payload = std::move( message.buffer)};
            }

            invoke::concurrent::Parameter parameter( common::message::service::call::callee::Request&& message,  std::function< void( service::invoke::Result&&)> callback)
            {
               Trace trace{ "manager::service::handle::detail::transform::parameter"};

               using parameter_flag = manager::service::invoke::Parameter::Flag;

               return invoke::concurrent::Parameter{ 
                  .invoke = { 
                     .flags = flag::convert( parameter_flag::no_reply, message.flags),
                     .service = std::move( message.service.name),
                     .payload = std::move( message.buffer)
                  },
                  .callback = std::move( callback)
               };
            }

         } // transform

         namespace callback
         {
            std::function< void( service::invoke::Result&&)> reply( 
               common::strong::ipc::id ipc, 
               common::message::service::call::Reply&& reply, 
               std::function< void( common::service::Code)> send_ack)
            {
               Trace trace{ "manager::service::handle::detail::callback::reply"};

               return [ ipc, reply = std::move( reply), send_ack = std::move( send_ack)]( service::invoke::Result&& result) mutable
               {
                  Trace trace{ "manager::service::handle::detail::callback::reply: callback"};

                  detail::complement_reply( std::move( result), reply);

                  send_ack( reply.code);

                  detail::send_reply( ipc, reply);
               };
            }


            std::function< void( service::invoke::Result&&)> no_reply( std::function< void( common::service::Code)> send_ack)
            {
               Trace trace{ "manager::service::handle::detail::callback::no_reply"};

               return [ send_ack = std::move( send_ack)]( service::invoke::Result&& result) mutable
               {
                  Trace trace{ "manager::service::handle::detail::callback::no_reply: callback"};

                  send_ack( local::transform_code( result.code));

                  // caller doesn't care about the reply, so we don't send it
               };
            }

         } // callback
         
      } // detail



   } // manager::service::handle

} // casual
