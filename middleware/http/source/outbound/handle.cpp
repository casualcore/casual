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
      namespace local
      {
         namespace
         {
            namespace metric::maybe
            {
               void send( State& state)
               {
                  if( state.pending.requests.empty() || state.metric)
                  {
                     communication::device::blocking::send( communication::instance::outbound::service::manager::device(), state.metric.message());
                     state.metric.clear();
                  }
               }
            } // metric::maybe
         } // <unnamed>
      } // local

      namespace internal
      {
         namespace local
         {
            namespace
            {
               namespace service::call
               {
                  namespace detail::send::error
                  {

                     void reply( State& state, message::service::call::callee::Request& message, code::xatmi code)
                     {
                        Trace trace{ "http::request::local::handle::service::call::detail::send::error_reply"};
                        log::line( log::category::verbose::error, code, " - message: ", message);

                        auto send_metric = []( auto& state, auto& message, code::xatmi code)
                        {
                           common::message::event::service::Metric metric;

                           metric.process = common::process::handle();
                           metric.correlation = message.correlation;
                           metric.execution = message.execution;
                           metric.service = message.service.logical_name();
                           metric.parent =  message.parent;
                           metric.type = decltype( metric.type)::concurrent;
                           metric.code = { .result = code};
                           
                           metric.trid = message.trid;
                           // This feels a bit meh?
                           metric.start = platform::time::clock::type::now();
                           metric.end = platform::time::clock::type::now();

                           state.metric.add( std::move( metric));
                           http::outbound::handle::local::metric::maybe::send( state);
                        };

                        auto reply = message::reverse::type( message);
                        reply.code.result = code;

                        // TODO should we really mess with the transaction? 
                        if( message.trid)
                           reply.transaction_state = decltype( reply.transaction_state)::rollback;

                        if( ! flag::contains( message.flags, message::service::call::request::Flag::no_reply))
                           communication::device::blocking::optional::send( message.process.ipc, std::move( reply));

                        send_metric( state, message, code);
                     }
                    
                  } // detail::send::error

                  auto request( State& state)
                  {
                     return [&state]( message::service::call::callee::Request& message)
                     {
                        Trace trace{ "http::outbound::handle::local::service::call::request"};
                        log::line( verbose::log, "message: ", message);

                        auto found = algorithm::find( state.lookup, message.service.name);

                        if( ! found)
                        {
                           log::line( log::category::error, code::xatmi::no_entry, " - http-outbound service not configured: ",  message.service.name);
                           detail::send::error::reply( state, message, code::xatmi::no_entry);
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
                              detail::send::error::reply( state, message, code::xatmi::protocol);
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

                        using Services = std::remove_reference_t< decltype( reply.content.services)>;
                        reply.content.services = algorithm::accumulate( message.content.services, Services{}, [ &state]( auto result, auto& name)
                        {
                           if( algorithm::find( state.lookup, name))
                           {
                              result.push_back( { 
                                 .name = name, 
                                 .category = "http", 
                                 .transaction = common::service::transaction::Type::none, 
                                 .visibility = common::service::visibility::Type::undiscoverable,
                                 .property = { .hops = 1}});
                           }
                           return result;
                        });

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
               local::discovery::request( state),
               // discard direct topology explore, since we've got nothing to explore
               common::message::dispatch::handle::discard< casual::domain::message::discovery::topology::direct::Explore>());
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
               message.transaction_state = request::transform::transaction( request, message.code);

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
               local::metric::maybe::send( state);
            };

         }
      } // external
      
   } // http::outbound::handle
} // casual