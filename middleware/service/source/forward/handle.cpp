//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "service/forward/handle.h"
#include "service/common.h"

#include "common/message/dispatch.h"
#include "common/message/dispatch/handle.h"

#include "common/server/handle/call.h"
#include "common/communication/instance.h"
#include "common/communication/ipc/flush/send.h"
#include "common/flag.h"


#include <iomanip>

namespace casual
{
   using namespace common;

   namespace service::forward::handle
   {

      namespace local
      {
         namespace
         {
            namespace send::error
            {
               void reply( State& state, const common::message::service::call::callee::Request& message, common::code::xatmi code)
               {
                  Trace trace{ "service::forward::handle::service::send::error::reply"};

                  if( ! flag::contains( message.flags, common::message::service::call::request::Flag::no_reply))
                  {
                     common::message::service::call::Reply reply;
                     reply.correlation = message.correlation;
                     reply.execution = message.execution;
                     reply.code.result = code;
                     reply.buffer = buffer::Payload{ nullptr};

                     state.multiplex.send( message.process.ipc, reply);
                  }

               }

            } // send::error
            
            namespace service
            {
               namespace lookup
               {
                  auto reply( State& state)
                  {
                     return [&state]( const message::service::lookup::Reply& message)
                     {
                        Trace trace{ "service::forward::handle::local::service::name::lookup::reply"};
                        log::line( verbose::log, "message: ", message);

                        auto found = algorithm::find( state.pending, message.correlation);

                        if( ! found)
                        {
                           log::line( log::category::error, "service lookup reply for a service '", message.service.name, "' has no registered call - action: discard");
                           return;
                        }

                        // We consume the request regardless

                        auto request = algorithm::container::extract( state.pending, std::begin( found));
                        request.service = message.service;

                        if( message.state == decltype( message.state)::absent)
                        {
                           log::line( log::category::error, code::casual::domain_instance_unavailable,  " service '", message.service.name, "' has no entry - action: send error reply");
                           send::error::reply( state, request, common::code::xatmi::no_entry);
                           return;
                        }

                        log::line( log, "send request - to: ", message.process, " - request: ", request);

                        state.multiplex.send( message.process.ipc, request, [ &state, request, service_name = message.service.name]( auto& destination, auto& complete)
                        {
                           log::line( log::category::error, "call to service ", std::quoted( service_name), " failed - action: send error reply");
                           send::error::reply( state, request, common::code::xatmi::no_entry);
                        });
                     };
                  }
               }

               namespace call
               {
                  auto request( State& state)
                  {
                     return [&state]( common::message::service::call::callee::Request& message)
                     {
                        Trace trace{ "service::forward::handle::local::service::request"};
                        log::line( verbose::log, "message: ", message);

                        // lookup service
                        {
                           message::service::lookup::Request request{ process::handle()};
                           request.requested = message.service.name;
                           request.correlation = message.correlation;
                           // semantic explicitly as a lookup from service-forward
                           request.context.semantic = decltype( request.context.semantic)::forward_request;
                           state.multiplex.send( communication::instance::outbound::service::manager::device(), request);
                        }

                        state.pending.push_back( std::move( message));
                     };

                  }
               }
            } // service

            namespace shutdown
            {
               auto request( State& state)
               {
                  return [&state]( const common::message::shutdown::Request& message)
                  {
                     Trace trace{ "service::forward::handle::local::shutdown::request"};
                     log::line( verbose::log, "message: ", message);

                     state.runlevel = state::Runlevel::shutdown;

                     auto send_discard = [&state]( auto& pending)
                     {
                        message::service::lookup::discard::Request request{ process::handle()};
                        request.reply = false;
                        request.correlation = pending.correlation;
                        state.multiplex.send( communication::instance::outbound::service::manager::device(), request);
                     };

                     algorithm::for_each( state.pending, send_discard);

                     for( auto& pending : state.pending)
                        send::error::reply( state, pending, common::code::xatmi::system);

                  };
               }
            }

         } // <unnamed>
      } // local
       
      dispatch_type create( State& state)
      {
         return dispatch_type{
            common::message::dispatch::handle::defaults( state),
            local::service::lookup::reply( state),
            local::service::call::request( state),
            local::shutdown::request( state)
         };
      }

   } // service::forward::handle
} // casual

