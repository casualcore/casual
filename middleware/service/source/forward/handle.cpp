//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "service/forward/handle.h"
#include "service/common.h"

#include "common/message/dispatch.h"
#include "common/message/dispatch/handle.h"

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

                  // We expect all call-requests to be "send and forget", so in practice we'll never send a reply.
                  // But i'll keep the code here for now, we need to be able to handle _forwards_ with replies (and in transaction)
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
                     return [ &state]( const message::service::lookup::Reply& message)
                     {
                        Trace trace{ "service::forward::handle::local::service::name::lookup::reply"};
                        log::debug( "message: ", message);

                        if( auto found = algorithm::find( state.pending, message.correlation))
                        {
                           // We consume the request regardless

                           auto request = algorithm::container::extract( state.pending, std::begin( found));
                            
                           // update the request with stuff from lookup-reply (span, deadline, etc)
                           request.update( message);

                            // If the service is idle, we can just forward the request. 
                            // If not, we'll reply to the caller with an error, as we don't want to deal with pending requests in this forwarder.

                           using Enum = decltype( message.state);
                           switch( message.state)
                           {
                              case Enum::idle:
                                 // The service is available, forward the request.
                                 state.multiplex.send( message.process.ipc, request, [ &state]( auto& destination, auto& complete)
                                 {
                                    auto request = serialize::native::complete< common::message::service::call::callee::Request>( complete);

                                    log::warning( code::casual::communication_invalid_address, "forward call to service ", std::quoted( request.service.name), ", destination: ", destination, " failed - action: error reply");
                                    send::error::reply( state, request, common::code::xatmi::no_entry);
                                 }); 

                                 break;
                              case Enum::absent:
                                 log::error( code::xatmi::no_entry, "service ", std::quoted( message.service.name), " is now absent - action: error reply");
                                 send::error::reply( state, request, code::xatmi::no_entry);

                                 break;
                              case Enum::timeout:
                                 log::error( code::xatmi::timeout, "service ", std::quoted( message.service.name), " lookup timed out - action: error reply");
                                 send::error::reply( state, request, code::xatmi::timeout);

                                 break;
                              default:
                                 log::error( code::casual::invalid_semantics, "unexpected state in lookup reply ", message, " - action: error reply");
                                 send::error::reply( state, request, code::xatmi::service_error);
                                 break;
                           }
                        }
                        else
                        {
                           log::error( code::casual::invalid_semantics, "service lookup reply for a service '", message.service.name, "' has no registered call - action: discard");
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
                        log::debug( "message: ", message);

                        // lookup service
                        {
                           message::service::lookup::Request request{ process::handle()};
                           request.correlation = message.correlation;
                           request.execution = message.execution;
                           request.requested = message.service.name;
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
                     log::debug( "message: ", message);

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

