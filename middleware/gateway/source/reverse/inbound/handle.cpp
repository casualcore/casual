//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/reverse/inbound/handle.h"

#include "gateway/message.h"

#include "common/communication/device.h"
#include "common/communication/instance.h"
#include "common/message/handle.h"


namespace casual
{
   using namespace common;

   namespace gateway::reverse::inbound::handle
   {   
      namespace local
      {
         namespace
         {
            namespace ipc
            {
               namespace manager
               {
                  auto& service() { return communication::instance::outbound::service::manager::device();}
                  auto& queue() { return communication::instance::outbound::queue::manager::device();}
                  auto& transaction() { return common::communication::instance::outbound::transaction::manager::device();}

               } // manager
               auto& inbound() { return communication::ipc::inbound::device();}
            } // ipc

            namespace tcp
            {
               template< typename M>
               void send( State& state, M&& message)
               {
                  try
                  {
                     if( auto connection = state.consume( message.correlation))
                     {
                        communication::device::blocking::send( 
                           connection->device,
                           std::forward< M>( message));

                        return;
                     }
                     
                     log::line( log::category::error, code::casual::communication_unavailable, " connection absent when trying to send reply - ", message.type());
                     log::line( log::category::verbose::error, code::casual::communication_unavailable, " message: ", message);
                  }
                  catch( ...)
                  {
                     exception::sink::error();
                  }
               }
               
            } // tcp

            namespace internal
            {
               template< typename Message>
               auto basic_forward( State& state)
               {
                  return [&state]( Message& message)
                  {
                     Trace trace{ "gateway::reverse::inbound::handle::local::basic_forward"};
                     common::log::line( verbose::log, "forward message: ", message);

                     tcp::send( state, message);
                  };
               }

               namespace connect
               {
                  //! pushed/forward from the external side, to enable correlation
                  auto request( State& state)
                  {
                     return [&state]( common::message::gateway::domain::connect::Request& message)
                     {
                        Trace trace{ "gateway::reverse::inbound::handle::local::internal::connect::request"};
                        common::log::line( verbose::log, "message: ", message);

                        auto reply = common::message::reverse::type( message);
                        if( common::algorithm::find( message.versions, decltype( reply.version)::version_1))
                           reply.version = decltype( reply.version)::version_1;

                        reply.domain = common::domain::identity();

                        tcp::send( state, reply);
                     };
                  }
               } // connect

               namespace call
               {
                  namespace lookup
                  {
                     auto reply( State& state)
                     {
                        return [&state]( common::message::service::lookup::Reply& message)
                        {
                           Trace trace{ "gateway::reverse::inbound::handle::local::internal::call::lookup::reply"};
                           common::log::line( verbose::log, "message: ", message);

                           auto send_error_reply = [&state, &message]( auto code)
                           {
                              common::message::service::call::Reply reply;
                              reply.correlation = message.correlation;
                              reply.code.result = code;
                              tcp::send( state, reply);
                           };

                           auto request = state.messages.consume( message.correlation, message.pending);

                           switch( message.state)
                           {
                              using Enum = decltype( message.state);
                              case Enum::idle:
                              {
                                 if( ! communication::device::blocking::optional::put( message.process.ipc, request))
                                 {
                                    log::line( common::log::category::error, common::code::xatmi::service_error, " server: ", message.process, " has been terminated during interdomain call - action: reply with: ", common::code::xatmi::service_error);
                                    send_error_reply( common::code::xatmi::service_error);
                                 }
                                 break;
                              }
                              case Enum::absent:
                              {
                                 log::line( common::log::category::error, common::code::xatmi::no_entry, " service: ", message.service, " is not handled by this domain (any more) - action: reply with: ", common::code::xatmi::no_entry);
                                 send_error_reply( common::code::xatmi::no_entry);
                                 break;
                              }
                              default:
                              {
                                 log::line( common::log::category::error, common::code::xatmi::service_error, " unexpected state on lookup reply: ", message, " - action: reply with: ", common::code::xatmi::service_error);
                                 send_error_reply( common::code::xatmi::service_error);
                                 break;
                              }
                           }
                        };  
                     }

                  } // lookup

                  auto reply = basic_forward< common::message::service::call::Reply>;

               } // call

               namespace queue
               {
                  namespace lookup
                  {
                     auto reply( State& state)
                     {
                        return [&state]( common::message::queue::lookup::Reply& message)
                        {
                           Trace trace{ "gateway::reverse::inbound::handle::local::internal::queue::lookup::reply"};
                           common::log::line( verbose::log, "message: ", message);

                           auto request = state.messages.consume( message.correlation);

                           if( message.process)
                           {
                              communication::device::blocking::put( message.process.ipc, request);
                              return;
                           }

                           // queue not available - send error reply
               
                           auto send_error = [&state, &message]( auto&& reply)
                           {
                              reply.correlation = message.correlation;
                              tcp::send( state, reply);
                           };

                           switch( request.type)
                           {
                              using Enum = decltype( request.type);
                              case Enum::queue_dequeue_request:
                              {
                                 send_error( common::message::queue::dequeue::Reply{});
                                 break;
                              }
                              case Enum::queue_enqueue_request:
                              {
                                 send_error( common::message::queue::enqueue::Reply{});
                                 break;
                              }
                              default:
                                 common::log::line( common::log::category::error, "unexpected message type for queue request: ", message, " - action: drop message");
                           }
                        };
                     }
                  } // lookup

                  namespace dequeue
                  {
                     auto reply = basic_forward< common::message::queue::dequeue::Reply>;
                  } // dequeue

                  namespace enqueue
                  {
                     auto reply = basic_forward< common::message::queue::enqueue::Reply>;
                  } // enqueue
               } // queue

               namespace domain
               {
                  namespace discover
                  {
                     auto reply( State& state)
                     {
                        return [&state]( common::message::gateway::domain::discover::Reply& message)
                        {
                           Trace trace{ "gateway::inbound::handle::internal::domain::discover::Reply"};
                           common::log::line( verbose::log, "message: ", message);

                           // Might send the accumulated message if all requested has replied.
                           state.coordinate.discovery( std::move( message));
                        };
                     }

                  } // discover
               } // domain


               namespace transaction
               {
                  namespace resource
                  {
                     namespace prepare
                     {
                        auto reply = basic_forward< common::message::transaction::resource::prepare::Reply>;
                     } // prepare
                     namespace commit
                     {
                        auto reply = basic_forward< common::message::transaction::resource::commit::Reply>;
                     } // commit
                     namespace rollback
                     {
                        auto reply = basic_forward< common::message::transaction::resource::rollback::Reply>;
                     } // commit
                  } // resource
               } // transaction

               namespace state
               {
                  auto request( State& state)
                  {
                     return [&state]( message::reverse::inbound::state::Request& message)
                     {
                        Trace trace{ "gateway::reverse::inbound::local::handle::internal::state::request"};
                        log::line( verbose::log, "message: ", message);

                        auto reply = common::message::reverse::type( message);
                        log::line( verbose::log, "state: ", state);

                        communication::device::blocking::optional::send( message.process.ipc, reply);
                     };
                  }

               } // state

            } // internal

            namespace external
            {
               namespace connect
               {
                  auto request( State& state)
                  {
                     return []( common::message::gateway::domain::connect::Request& message)
                     {
                        Trace trace{ "gateway::reverse::inbound::handle::local::external::connect::request"};
                        common::log::line( verbose::log, "message: ", message);

                        // we push it to the internal side, so it's possible to correlate the message.
                        // We don't know which external socket has sent this message at this moment.
                        // when we're done here, the 'external-dispatch' will correlate the socket descriptor
                        // and the correlation id of the message, so we can access it from the internal part.
                        ipc::inbound().push( std::move( message));
                     };
                  }
               } // connect

               namespace call
               {
                  auto request( State& state)
                  {
                     return [&state]( common::message::service::call::callee::Request& message)
                     {
                        Trace trace{ "gateway::reverse::inbound::handle::local::external::call::request"};
                        common::log::line( verbose::log, "message: ", message);

                        // Change 'sender' so we get the reply
                        message.process = common::process::handle();

                        // Prepare lookup
                        common::message::service::lookup::Request request;
                        {
                           request.correlation = message.correlation;
                           request.requested = message.service.name;
                           request.context = decltype( request.context)::no_busy_intermediate;
                           request.process = common::process::handle();
                        }

                        // Add message to buffer
                        state.messages.add( std::move( message));

                        // Send lookup
                        communication::device::blocking::send( ipc::manager::service(), request);  
                     };
                  }
               } // call

               namespace queue
               {
                  namespace lookup
                  {
                     template< typename M>
                     bool send( State& state, M&& message)
                     {
                        Trace trace{ "gateway::reverse::inbound::handle::local::external::queue::lookup::send"};

                        // Prepare queue lookup
                        common::message::queue::lookup::Request request{ common::process::handle()};
                        {
                           request.correlation = message.correlation;
                           request.name = message.name;
                        }

                        if( communication::device::blocking::optional::send( ipc::manager::queue(), request))
                        {
                           // Change 'sender' so we get the reply
                           message.process = common::process::handle();

                           // Add message to buffer
                           state.messages.add( std::forward< M>( message));

                           return true;
                        }

                        return false;

                     }
                  } // lookup

                  namespace enqueue
                  {
                     auto request( State& state)
                     {
                        return [&state]( common::message::queue::enqueue::Request& message)
                        {
                           Trace trace{ "gateway::reverse::inbound::handle::local::external::queue::enqueue::Request"};

                           common::log::line( verbose::log, "message: ", message);

                           // Send lookup
                           if( ! queue::lookup::send( state, message))
                           {
                              common::log::line( common::log::category::error, "failed to lookup queue - action: send error reply");

                              common::message::queue::enqueue::Reply reply;
                              reply.correlation = message.correlation;
                              reply.execution = message.execution;

                              // empty uuid represent error. TODO: is this enough? No, it is not.
                              reply.id = common::uuid::empty();

                              tcp::send( state, reply);
                           }
                        };
                     }
                  } // enqueue

                  namespace dequeue
                  {
                     auto request( State& state)
                     {
                        return [&]( common::message::queue::dequeue::Request& message)
                        {
                           Trace trace{ "gateway::inbound::handle::queue::dequeue::Request::operator()"};

                           common::log::line( verbose::log, "message: ", message);

                           // Send lookup
                           if( ! queue::lookup::send( state, message))
                           {
                              common::log::line( common::log::category::error, "failed to lookup queue - action: send error reply");

                              common::message::queue::enqueue::Reply reply;
                              reply.correlation = message.correlation;
                              reply.execution = message.execution;

                              // empty uuid represent error. TODO: is this enough? No, it is not.
                              reply.id = common::uuid::empty();

                              tcp::send( state, reply);
                           }
                        };
                     }
                  } // dequeue
               } // queue

               namespace domain
               {
                  namespace discover
                  {
                     auto request( State& state)
                     {
                        return [&state]( common::message::gateway::domain::discover::Request& message)
                        {
                           Trace trace{ "gateway::reverse::inbound::handle::local::external::connection::discover::request"};
                           common::log::line( verbose::log, "message: ", message);
                           
                           // Make sure we get the reply
                           message.process = common::process::handle();

                           // make sure we get unique correlations for the coordination
                           auto correlation = std::exchange( message.correlation, {});

                           // Forward to service-manager and possible queue-manager

                           std::vector< state::coordinate::discovery::Message::Pending> pending;

                           if( ! message.services.empty())
                           {
                              pending.emplace_back(  
                                 communication::device::blocking::send( ipc::manager::service(), message),
                                 ipc::manager::service().connector().process().pid);
                           }

                           if( ! message.queues.empty())
                           {
                              if( auto correlation = communication::device::blocking::optional::send( ipc::manager::queue(), message))
                              {
                                 pending.emplace_back(  
                                    correlation,
                                    ipc::manager::service().connector().process().pid);
                              }
                           }

                           state.coordinate.discovery( std::move( pending), [&state, correlation]( auto received, auto failed)
                           {
                              Trace trace{ "gateway::reverse::inbound::handle::local::external::connection::discover::request coordinate"};

                              common::message::gateway::domain::discover::Reply message;
                              message.correlation = correlation;
                              message.domain = common::domain::identity();

                              algorithm::for_each( received, [&message]( auto& reply)
                              {
                                 algorithm::append( reply.services, message.services);
                                 algorithm::append( reply.queues, message.queues);

                              });

                              tcp::send( state, message);
                           });
                           
                        };
                     }
                  } // discover
               } // domain


               namespace transaction
               {
                  namespace resource
                  {
                     template< typename Message>
                     auto basic_request()
                     {
                        return []( Message& message)
                        {
                           // Set 'sender' so we get the reply
                           message.process = common::process::handle();
                           communication::device::blocking::send( ipc::manager::transaction(), message);
                        };
                     }

                     namespace prepare
                     {
                        auto request = basic_request< common::message::transaction::resource::prepare::Request>;
                     } // prepare
                     namespace commit
                     {
                        auto request = basic_request< common::message::transaction::resource::commit::Request>;
                     } // commit
                     namespace rollback
                     {
                        auto request = basic_request< common::message::transaction::resource::rollback::Request>;
                     } // commit
                  } // resource
               } // transaction
            } // external

         } // <unnamed>
      } // local
      internal_handler internal( State& state)
      {
         return {
            common::message::handle::defaults( communication::ipc::inbound::device()),

            local::internal::connect::request( state),

            // service call
            local::internal::call::lookup::reply( state),
            local::internal::call::reply( state),

            // queue
            local::internal::queue::lookup::reply( state),
            local::internal::queue::dequeue::reply( state),
            local::internal::queue::enqueue::reply( state),

            // transaction
            local::internal::transaction::resource::prepare::reply( state),
            local::internal::transaction::resource::commit::reply( state),
            local::internal::transaction::resource::rollback::reply( state),

            // domain discovery
            local::internal::domain::discover::reply( state),

            local::internal::state::request( state)
         };
      }

      external_handler external( State& state)
      {
         return {
            
            // just a forward to the internal connect::request
            local::external::connect::request( state),

            // service call
            local::external::call::request( state),

            // queue
            local::external::queue::enqueue::request( state),
            local::external::queue::dequeue::request( state),

            // domain discover
            local::external::domain::discover::request( state),

            // transaction
            local::external::transaction::resource::prepare::request(),
            local::external::transaction::resource::commit::request(),
            local::external::transaction::resource::rollback::request()
         };
      }
      
      
   } // gateway::reverse::inbound::handle

} // casual