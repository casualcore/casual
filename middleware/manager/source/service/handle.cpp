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

         bool send_reply( common::message::service::call::request::Flag flags)
         {
            return ! flag::contains( flags, common::message::service::call::request::Flag::no_reply);
         }


         common::message::service::call::ACK prepare_ack( const common::message::service::call::callee::Request& message)
         {
            Trace trace{ "manager::service::handle::detail::prepare_ack"};

            common::message::service::call::ACK result;
            result.correlation = message.correlation;
            result.execution = message.execution;

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

            reply.code.result = result.code.result == decltype( result.code.result)::success ? common::code::xatmi::ok : common::code::xatmi::service_fail;
            reply.code.user = result.code.user;
            reply.buffer = std::move( result.payload);
         }

         namespace transform
         {
            invoke::Parameter parameter( common::message::service::call::callee::Request&& message)
            {
               Trace trace{ "manager::service::handle::detail::transform::parameter"};

               using parameter_flag = manager::service::invoke::Parameter::Flag;

               return invoke::Parameter{ 
                  .flags = flag::convert( parameter_flag::no_reply, message.flags),
                  .service = std::move( message.service.name),
                  .header = std::move( message.header),
                  .payload = std::move( message.buffer)};
            }


            
         } // transform
         
      } // detail



   } // manager::service::handle

} // casual