//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "http/outbound/handle.h"
#include "http/outbound/request.h"
#include "http/common.h"

#include "domain/message/discovery.h"

#include "common/communication/instance.h"
#include "common/communication/ipc/flush/send.h"
#include "common/message/dispatch/handle.h"

namespace casual
{
   using namespace common;
   namespace http::outbound::handle
   {
      namespace internal
      {
         namespace local
         {
            namespace
            {
               namespace service::call
               {
                  auto request( State& state)
                  {
                     return [&state]( message::service::call::callee::Request& message)
                     {
                        Trace trace{ "http::outbound::handle::local::service::call::request"};
                        log::line( verbose::log, "message: ", message);

                        auto send_error_reply = []( auto& message, auto code)
                        {
                           Trace trace{ "http::request::local::handle::service::call::Request::send_error_reply"};
                           log::line( log::category::verbose::error, code, " - message: ", message);

                           auto reply = message::reverse::type( message);
                           reply.code.result = code;

                           if( message.trid)
                           {
                              reply.transaction.trid = message.trid;
                              reply.transaction.state = decltype( reply.transaction.state)::rollback;
                           }

                           if( ! message.flags.exist( message::service::call::request::Flag::no_reply))
                              communication::device::blocking::optional::send( message.process.ipc, std::move( reply));
                        };

                        auto found = algorithm::find( state.lookup, message.service.name);

                        if( ! found)
                        {
                           log::line( log::category::error, code::xatmi::no_entry, " - http-outbound service not configured: ",  message.service.name);
                           send_error_reply( message, code::xatmi::no_entry);
                           return;
                        }

                        const auto& node = found->second;

                        if( message.trid)
                        {
                           if( ! node.discard_transaction)
                           {
                              // we can't allow this forward to be in a transaction
                              log::line( log::category::error, code::xatmi::protocol, " - http-outbound can't be used in transaction for service: ",  message.service.name);
                              log::line( log::category::verbose::error, code::xatmi::protocol, " - node: ", node);
                              send_error_reply( message, common::code::xatmi::protocol);
                              return;
                           }
                           else
                              log::line( log, "transaction discarded for '", message.service.name, "'");
                        }

                        // prepare and add the curl call
                        state.pending.requests.add( request::prepare( node, std::move( message)));
                     };
                  }
               } // service::call

               namespace discovery
               {
                  //! reply with the intersection of requested and what we got...
                  auto request( const State& state)
                  {
                     return [&state]( casual::domain::message::discovery::Request& message)
                     {
                        Trace trace{ "http::outbound::handle::local::discovery::request"};
                        log::line( verbose::log, "message: ", message);

                        auto reply = common::message::reverse::type( message);
                        reply.domain = state.identity;

                        using Services = std::remove_reference_t< decltype( reply.content.services())>;
                        reply.content.services( algorithm::accumulate( message.content.services(), Services{}, [&state]( auto result, auto& name)
                        {
                           if( algorithm::find( state.lookup, name))
                           {
                              auto& service = result.emplace_back( name, "http", common::service::transaction::Type::none);
                              service.property.hops = 1;
                              service.property.type = decltype( service.property.type)::configured;
                           }
                           return result;
                        }));

                        communication::ipc::flush::optional::send( message.process.ipc, reply);
                     };
                  }

               } // discovery

            } // <unnamed>
         } // local

         handler_type create( State& state)
         {
            Trace trace{ "http::outbound::handle::handle"};

            auto& device = communication::ipc::inbound::device();
            return message::dispatch::handler( device,
               message::dispatch::handle::defaults( state),
               local::service::call::request( state),
               local::discovery::request( state));
         }
      } // internal

      namespace external
      {
         common::function< void( state::pending::Request&&, curl::type::code::easy)> reply( State& state)
         {
            return [&state]( state::pending::Request&& request, curl::type::code::easy curl_code)
            {
               Trace trace{ "http::outbound::manager::local::handle::Reply"};

               log::line( verbose::log, "request: ", request);
               log::line( verbose::log, "curl_code: ", curl_code);
               
               message::service::call::Reply message;
               message.correlation = request.state().correlation;
               message.execution = request.state().execution;
               message.code = request::transform::code( request, curl_code);
               message.transaction = request::transform::transaction( request, message.code);

               // take care of metrics
               state.metric.add( request, message.code);

               auto destination = request.state().destination;

               // we're done with the request.

               // there is only a payload if the call was 'successful'...
               if( algorithm::compare::any( message.code.result, code::xatmi::ok, code::xatmi::service_fail))
               {
                  try
                  {
                     message.buffer = request::receive::payload( std::move( request));
                  }
                  catch( ...)
                  {
                     auto error = exception::capture();
                     log::line( log::category::verbose::error, common::code::xatmi::protocol, " failed to transcode payload - reason: ", error);
                     message.code.result = common::code::xatmi::protocol; 
                  }
               }

               log::line( verbose::log, "message: ", message);

               communication::device::blocking::optional::send( destination.ipc, message);

               // do we send metrics to service-manager?
               if( state.pending.requests.empty() || state.metric)
               {
                  communication::device::blocking::send( communication::instance::outbound::service::manager::device(), state.metric.message());
                  state.metric.clear();
               }
            };

         }
      } // external
      
   } // http::outbound::handle
} // casual