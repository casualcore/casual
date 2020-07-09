//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "http/outbound/manager.h"
#include "http/outbound/configuration.h"
#include "http/outbound/transform.h"
#include "http/outbound/request.h"
#include "http/common.h"

#include "common/communication/instance.h"
#include "common/message/service.h"
#include "common/message/handle.h"
#include "common/exception/handle.h"

namespace casual
{
   using namespace common;

   namespace http
   {
      namespace outbound
      {
         namespace manager
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
                           return ! communication::device::non::blocking::send( process.ipc, message).empty();
                        }
                        catch( const exception::system::communication::Unavailable&)
                        {
                           log::line( log, "failed to send message - type: ", common::message::type( message), " to: ", process, " - action: ignore");
                        }
                        return true;
                     }

                     namespace optional
                     {
                        template< typename M>
                        void send( State& state, const process::Handle& process, M&& message)
                        {
                           try
                           {
                              communication::device::blocking::send( process.ipc, message);
                              /*
                              if( ! communication::device::non::blocking::send( process.ipc, message))
                              {
                                 log::line( verbose::log, "failed to send message - type: ", common::message::type( message), " to: ", process, " - action: try later");
                                 state.pending.replies.emplace_back( std::move( message), process);
                              }
                              */
                           }
                           catch( const exception::system::communication::Unavailable&)
                           {
                              log::line( log, "failed to send message - type: ", common::message::type( message), " to: ", process, " - action: ignore");
                           }
                        }
                     } // optional
                  } // ipc
                  namespace handle
                  {
                     namespace service
                     {
                        namespace call
                        {
                           auto request( State& state)
                           {
                              return [&state]( message::service::call::callee::Request& message)
                              {
                                 Trace trace{ "http::outbound::manager::local::handle::service::call::Request"};
                                 log::line( verbose::log, "message: ", message);

                                 auto send_error_reply = [&state]( auto& message, auto code)
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
                                       local::ipc::optional::send( state, message.process, std::move( reply));
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
                        } // call
                     } // service

                     auto reply( State& state)
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
                                 message.buffer = request::receive::transcode::payload( std::move( request));
                              }
                              catch( const std::exception& exception)
                              {
                                 log::line( log::category::verbose::error, "failed to transcode payload: ", exception);
                                 message.code.result = common::code::xatmi::protocol; 
                              }
                           }

                           log::line( verbose::log, "message: ", message);

                           manager::local::ipc::optional::send( state, destination, message);

                           // do we send metrics to service-manager?
                           if( state.pending.requests.empty() || state.metric)
                           {
                              communication::device::blocking::send( common::communication::instance::outbound::service::manager::device(), state.metric.message());
                              state.metric.clear();
                           }
                        };
                     }
                  } // handle

                  namespace inbound
                  {
                     auto handlers( State& state)
                     {
                        auto& device = communication::ipc::inbound::device();
                        return message::dispatch::handler( device,
                           message::handle::defaults( device),
                           handle::service::call::request( state));
                     }
                  } // inbound

                  void advertise( const State& state)
                  {
                     Trace trace{ "http::outbound::manager::local::advertise"};

                     common::message::service::concurrent::Advertise message;
                     message.process = process::handle();

                     // highest possible order
                     message.order = std::numeric_limits< std::decay_t< decltype( message.order)>>::max();

                     algorithm::transform( state.lookup, message.services.add, []( auto& l){
                        message::service::concurrent::advertise::Service service;
                        service.category = "http";
                        service.name = l.first;
                        service.transaction = service::transaction::Type::none;
                        return service;
                     });

                     log::line( verbose::log, "advertise: ", message);

                     communication::device::blocking::send( communication::instance::outbound::service::manager::device(), message);
                  }
                  
               } // <unnamed>
            } // local


         } // manager

         Manager::Manager( manager::Settings settings) 
            : m_state( transform::configuration( configuration::get( settings.configurations)))
         {

            // connect to domain
            common::communication::instance::connect();

            manager::local::advertise( m_state);
         }

         Manager::~Manager()
         {
            Trace trace{ "http::outbound::Manager::~Manager"};

            try
            {
               log::line( log::category::information, "pending.requests.size: ", m_state.pending.requests.size(), 
                  ", pending.requests.capacity: ", m_state.pending.requests.capacity());

               auto send_error_reply = []( const auto& pending)
               {
                  message::service::call::Reply message;
                  message.correlation = pending.state().correlation;
                  message.execution = pending.state().execution;
                  message.code.result = code::xatmi::service_error;

                  manager::local::ipc::send( pending.state().destination, message);               
               };

               algorithm::for_each( m_state.pending.requests, send_error_reply);
            }
            catch( ...)
            {
               exception::handle();
            }

         }


         void Manager::run()
         {

            Trace trace{ "http::outbound::handle::run"};

            auto dispatch = manager::local::inbound::handlers( m_state);
            auto& ipc = communication::ipc::inbound::device();

            auto inbound = [&]( auto policy){
               dispatch( ipc.next( policy));
            };

            auto outbound = manager::local::handle::reply( m_state);

            while( true)
            {
               if( m_state.pending.requests)
               {
                  log::line( verbose::log, "state.pending.requests.size(): ", m_state.pending.requests.size());
                  request::blocking::dispath( m_state, inbound, outbound);
               }
               else
               {
                  // we've got no pending request, we only have to listen to inbound
                  inbound( communication::device::policy::blocking( ipc));
               } 
            }
         }

      } // outbound
   } // http
} // casual
