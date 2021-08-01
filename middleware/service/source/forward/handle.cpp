//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "service/forward/handle.h"
#include "service/common.h"

#include "common/message/dispatch.h"
#include "common/message/handle.h"

#include "common/server/handle/call.h"
#include "common/communication/instance.h"
#include "common/flag.h"

#include "domain/pending/message/send.h"

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
            namespace ipc
            {
               template< typename M> 
               bool send( const process::Handle& process, M&& message)
               {

                  try
                  {
                     if( ! communication::device::non::blocking::send( process.ipc, message))
                     {
                        log::line( log, "could not send message ", message.type(), " to process: ", process, " - action: eventually send");
                        casual::domain::pending::message::send( process, message);
                     }
                     return true;
                  }
                  catch( ...)
                  {
                     auto error = exception::capture();
                     if( error.code() != code::casual::communication_unavailable)
                        throw;

                     // No-op, we just drop the message
                     log::line( log, error, " failed to sendmessage ", message.type(), " to process: ", process, " - action: discard");
                     return true;
                  }
               }

            } // ipc

            namespace send::error
            {
               void reply( State& state, common::message::service::call::callee::Request& message, common::code::xatmi code)
               {
                  Trace trace{ "service::forward::handle::service::send::error::reply"};

                  if( ! message.flags.exist( common::message::service::call::request::Flag::no_reply))
                  {
                     common::message::service::call::Reply reply;
                     reply.correlation = message.correlation;
                     reply.execution = message.execution;
                     reply.transaction.trid = message.trid;
                     reply.code.result = code;
                     reply.buffer = buffer::Payload{ nullptr};
                     
                     local::ipc::send( message.process, reply);
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

                        if( message.state == message::service::lookup::Reply::State::busy)
                        {
                           log::line( log, "service is busy - action: wait for idle");
                           return;
                        }

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

                        if( ! local::ipc::send( message.process, request))
                        {
                           log::line( log::category::error, "call to service ", std::quoted( message.service.name), "' failed - action: send error reply");
                           send::error::reply( state, request, common::code::xatmi::no_entry);
                           return;
                        }
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
                           communication::device::blocking::send( communication::instance::outbound::service::manager::device(), request);
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

                     auto send_discard = []( auto& pending)
                     {
                        message::service::lookup::discard::Request request{ process::handle()};
                        request.reply = false;
                        request.correlation = pending.correlation;
                        communication::device::blocking::optional::send( communication::instance::outbound::service::manager::device(), request);
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
         return {
            local::service::lookup::reply( state),
            local::service::call::request( state),
            local::shutdown::request( state)
         };
      }

   } // service::forward::handle
} // casual




