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

            void finalize();
            
            //! @return true if we should send a reply
            bool caller_wants_reply( common::message::service::call::request::Flag flags);
            void send_reply( const common::strong::ipc::id& ipc, const common::message::service::call::Reply& reply);

            common::message::service::call::ACK prepare_ack( const common::message::service::call::callee::Request& message, common::chronology::time_point start);
            common::message::service::call::Reply prepare_reply( const common::message::service::call::callee::Request& message);
            void complement_reply( invoke::Result&& result, common::message::service::call::Reply& reply);

            namespace transform
            {
               invoke::Parameter parameter( common::message::service::call::callee::Request&& message);
               invoke::concurrent::Parameter parameter( common::message::service::call::callee::Request&& message,  std::function< void( service::invoke::Result&&)> callback);
               
            } // transform

            namespace callback
            {
               std::function< void( service::invoke::Result&&)> reply( common::strong::ipc::id ipc, common::message::service::call::Reply&& reply, std::function< void( common::service::Code)> send_ack);

               std::function< void( service::invoke::Result&&)> no_reply( std::function< void( common::service::Code)> send_ack);
            } // callback


            template< typename Policy>
            auto call( const context::State& state, common::message::service::call::callee::Request&& message)
            {
               common::Trace trace{ "manager::service::handle::detail::call"};
               common::log::debug( "message: ", message);

               auto start = common::chronology::time_point::clock::now();

               detail::set_execution_context( message);

               auto reply = detail::prepare_reply( message);

               auto send_reply_guard = common::execute::scope( [ &reply, ipc = message.process.ipc, flags = message.flags]()
               {
                  if( detail::caller_wants_reply( flags))
                     detail::send_reply( ipc, reply);

                  detail::finalize();
               });

               auto send_ack = [ ack = detail::prepare_ack( message, start)]( common::service::Code code) mutable
               {
                  ack.metric.end = platform::time::clock::type::now();
                  ack.metric.code.result = code.result;
                  ack.metric.code.user = code.user;
                  Policy::send_ack( ack);
               };

               auto send_ack_guard = common::execute::scope( [ &send_ack, &reply]()
               {
                  send_ack( reply.code);
               });


               if( auto found = common::algorithm::find( state.services, message.service.name))
               {
                  if( auto service = std::get_if< sequential::Service>( &found->second))
                  {
                     auto result = service->function( detail::transform::parameter( std::move( message)));
                     detail::complement_reply( std::move( result), reply);
                  }
                  else if( auto service = std::get_if< concurrent::Service>( &found->second))
                  {
                     auto flags = message.flags;
                     auto ipc = message.process.ipc;

                     if( detail::caller_wants_reply( flags))
                        service->function( detail::transform::parameter( std::move( message), detail::callback::reply( ipc, std::move( reply), std::move( send_ack))));
                     else
                        service->function( detail::transform::parameter( std::move( message), detail::callback::no_reply( std::move( send_ack))));


                     send_ack_guard.release();
                     detail::finalize();
                     send_reply_guard.release();
                  }
                  else
                  {
                     casual::terminate( "service is neither sequential nor concurrent: ", message.service.name);
                  }
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
