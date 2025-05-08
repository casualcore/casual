//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/manager/service/invoke.h"
#include "casual/manager/service/context/state.h"

#include "common/message/dispatch.h"
#include "common/message/service.h"
#include "common/communication/ipc.h"
#include "common/execute.h"


#include "common/log.h"

namespace casual
{
   namespace manager::service
   {   
      namespace handle
      {
         using dispatch_type = common::message::dispatch::basic_handler< common::communication::ipc::message::Complete>;

         namespace detail
         {
            void set_execution_context( const common::message::service::call::callee::Request& message);
            
            //! @return true if we should send a reply
            bool send_reply( common::message::service::call::request::Flag flags);

            common::message::service::call::ACK prepare_ack( const common::message::service::call::callee::Request& message);
            common::message::service::call::Reply prepare_reply( const common::message::service::call::callee::Request& message);
            void complement_reply( invoke::Result&& result, common::message::service::call::Reply& reply);

            namespace transform
            {
               invoke::Parameter parameter( common::message::service::call::callee::Request&& message);
               
            } // transform

            template< typename Policy>
            auto call( const context::State& state, common::message::service::call::callee::Request&& message)
            {
               common::Trace trace{ "manager::service::handle::detail::call"};

               auto start = platform::time::clock::type::now();

               detail::set_execution_context( message);

               auto reply = detail::prepare_reply( message);

               auto send_reply_guard = common::execute::scope( [ &reply, ipc = message.process.ipc, flags = message.flags]()
               {
                  if( detail::send_reply( flags))
                     common::communication::device::blocking::send( ipc, reply);
               });

               auto ack = detail::prepare_ack( message);
               ack.metric.start = start;

               auto send_ack_guard = common::execute::scope( [ &ack, &reply]()
               {
                  ack.metric.end = platform::time::clock::type::now();
                  ack.metric.code = reply.code;
                  Policy::send_ack( ack);
               });

               if( auto found = common::algorithm::find( state.services, message.service.name))
               {
                  auto result = found->second( detail::transform::parameter( std::move( message)));
                  detail::complement_reply( std::move( result), reply);
               }
               else
               {
                  reply.code.result = common::code::xatmi::system;
                  common::code::raise::error( common::code::xatmi::system, message.service.name, " not present at server - inconsistency between service-manager and server");
               }
            }
            
         } // detail
         
         template< typename Policy>
         auto call( const context::State& state)
         {
            return [ &state]( common::message::service::call::callee::Request&& message)
            {  
               try
               {
                  detail::call< Policy>( state, std::move( message));
               }
               catch( ...)
               {
                  auto error = common::exception::capture();
                  common::log::error( error);
               }                              
            };


         }

      } // handle

   } // manager::service

} // casual