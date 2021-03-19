//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/inbound/handle.h"
#include "gateway/group/ipc.h"

#include "gateway/message.h"
#include "gateway/common.h"

#include "domain/discovery/api.h"

#include "common/communication/device.h"
#include "common/communication/instance.h"
#include "common/message/handle.h"
#include "common/event/listen.h"


namespace casual
{
   using namespace common;

   namespace gateway::group::inbound::handle
   {   
      namespace local
      {
         namespace
         {
            namespace tcp
            {
               template< typename M>
               strong::file::descriptor::id send( State& state, M&& message)
               {
                  try
                  {
                     if( auto connection = state.consume( message.correlation))
                     {
                        connection->send( state, std::forward< M>( message));
                        return connection->descriptor();
                     }
                     
                     log::line( log, code::casual::communication_unavailable, " connection absent when trying to send reply - ", message.type());
                     log::line( verbose::log, code::casual::communication_unavailable, " message: ", message);
                  }
                  catch( ...)
                  {
                     exception::sink::error();
                  }
                  return {};
               }
               
            } // tcp

            namespace internal
            {
               template< typename Message>
               auto basic_forward( State& state)
               {
                  return [&state]( Message& message)
                  {
                     Trace trace{ "gateway::group::inbound::handle::local::basic_forward"};
                     common::log::line( verbose::log, "forward message: ", message);

                     tcp::send( state, message);
                  };
               }

               namespace connect
               {
                  //! pushed/forward from the external side, to enable correlation
                  auto request( State& state)
                  {
                     return [&state]( gateway::message::domain::connect::Request& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::internal::connect::request"};
                        common::log::line( verbose::log, "message: ", message);

                        auto reply = common::message::reverse::type( message);

                        if( auto found = algorithm::find_first_of( message::domain::protocol::versions, message.versions))
                           reply.version = *found;

                        reply.domain = common::domain::identity();

                        if( auto connection = state.external.connection( tcp::send( state, reply)))
                        {
                           connection->protocol = reply.version;
                           if( auto found = state.external.information( connection->descriptor()))
                              found->domain = message.domain;

                           common::log::line( verbose::log, "connection: ", *connection);
                        }
                     };
                  }
               } // connect

               namespace service
               {
                  namespace lookup
                  {
                     auto reply( State& state)
                     {
                        return [&state]( common::message::service::lookup::Reply& message)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::internal::call::lookup::reply"};
                           common::log::line( verbose::log, "message: ", message);

                           auto send_error_reply = [&state, &message]( auto code)
                           {
                              common::message::service::call::Reply reply;
                              reply.correlation = message.correlation;
                              reply.code.result = code;
                              tcp::send( state, reply);
                           };

                           auto request = state.pending.requests.consume( message.correlation, message.pending);

                           switch( message.state)
                           {
                              using Enum = decltype( message.state);
                              case Enum::idle:
                              {
                                 if( ! ipc::flush::optional::send( message.process.ipc, request))
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
                                 log::line( common::log::category::error, common::code::xatmi::system, " unexpected state on lookup reply: ", message, " - action: reply with: ", common::code::xatmi::service_error);
                                 send_error_reply( common::code::xatmi::system);
                                 break;
                              }
                           }
                        };  
                     }

                  } // lookup

                  namespace call
                  {
                     auto reply = basic_forward< common::message::service::call::Reply>;
                  } // call

               } // service

               namespace queue
               {
                  namespace lookup
                  {
                     auto reply( State& state)
                     {
                        return [&state]( casual::queue::ipc::message::lookup::Reply& message)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::internal::queue::lookup::reply"};
                           common::log::line( verbose::log, "message: ", message);

                           auto request = state.pending.requests.consume( message.correlation);

                           if( message.process)
                           {
                              ipc::flush::send( message.process.ipc, request);
                              return;
                           }

                           // queue not available - send error reply
               
                           auto send_error = [&state, &message]( auto&& reply)
                           {
                              reply.correlation = message.correlation;
                              tcp::send( state, reply);
                           };

                           switch( request.type())
                           {
                              using Enum = decltype( request.type());
                              case Enum::queue_group_dequeue_request:
                              {
                                 send_error( casual::queue::ipc::message::group::dequeue::Reply{});
                                 break;
                              }
                              case Enum::queue_group_enqueue_request:
                              {
                                 send_error( casual::queue::ipc::message::group::enqueue::Reply{});
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
                     auto reply = basic_forward< casual::queue::ipc::message::group::dequeue::Reply>;
                  } // dequeue

                  namespace enqueue
                  {
                     auto reply = basic_forward< casual::queue::ipc::message::group::enqueue::Reply>;
                  } // enqueue
               } // queue

               namespace domain
               {
                  namespace discovery
                  {
                     auto request( State& state)
                     {
                        return [&state]( gateway::message::domain::discovery::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::internal::domain::discovery::request"};
                           common::log::line( verbose::log, "message: ", message);

                           if( auto found = algorithm::find( state.correlations, message.correlation))
                           {
                              common::log::line( verbose::log, "correlation: ", *found);

                              // Set 'sender' so we get the reply
                              message.process = common::process::handle();

                              auto information = state.external.information( found->descriptor);
                              assert( information);

                              if( information->configuration.discovery == decltype( information->configuration.discovery)::forward)
                                  message.directive = decltype( message.directive)::forward;

                              casual::domain::discovery::request( message);
                           }
                        };
                     }

                     auto reply = basic_forward< gateway::message::domain::discovery::Reply>;

                  } // discovery
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

            } // internal

            namespace external
            {
               namespace connect
               {
                  auto request( State& state)
                  {
                     return []( gateway::message::domain::connect::Request&& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::external::connect::request"};
                        common::log::line( verbose::log, "message: ", message);

                        // we push it to the internal side, so it's possible to correlate the message.
                        // We don't know which external socket has sent this message at this moment.
                        // when we're done here, the 'external-dispatch' will correlate the socket descriptor
                        // and the correlation id of the message, so we can access it from the internal part.
                        ipc::inbound().push( std::move( message));
                     };
                  }
               } // connect

               namespace disconnect
               {
                  auto reply( State& state)
                  {
                     return [&state]( const gateway::message::domain::disconnect::Reply& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::external::disconnect::reply"};
                        common::log::line( verbose::log, "message: ", message);

                        if( auto connection = state.consume( message.correlation))
                        {
                           common::log::line( verbose::log, "connection: ", *connection);

                           auto descriptor = connection->descriptor();

                           if( algorithm::find( state.correlations, descriptor))
                           {
                              // connection still got pending stuff to do, we keep track until its 'idle'.
                              state.pending.disconnects.push_back( descriptor);
                           }
                           else 
                           {
                              // connection is 'idle', we just 'loose' the connection
                              handle::connection::lost( state, descriptor);
                           }
                        }
                     };
                  }
                  
               } // disconnect

               namespace service
               {
                  namespace call
                  {
                     auto request( State& state)
                     {
                        return [&state]( common::message::service::call::callee::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::external::service::call::request"};
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
                           state.pending.requests.add( std::move( message));

                           // Send lookup
                           ipc::flush::send( ipc::manager::service(), request);  
                        };
                     }
                  } // call
               } // service

               namespace queue
               {
                  namespace lookup
                  {
                     template< typename M>
                     bool send( State& state, M&& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::external::queue::lookup::send"};

                        // Prepare queue lookup
                        casual::queue::ipc::message::lookup::Request request{ common::process::handle()};
                        {
                           request.correlation = message.correlation;
                           request.name = message.name;
                        }

                        if( ipc::flush::optional::send( ipc::manager::optional::queue(), request))
                        {
                           // Change 'sender' so we get the reply
                           message.process = common::process::handle();

                           // Add message to buffer
                           state.pending.requests.add( std::forward< M>( message));

                           return true;
                        }

                        return false;

                     }
                  } // lookup

                  namespace enqueue
                  {
                     auto request( State& state)
                     {
                        return [&state]( casual::queue::ipc::message::group::enqueue::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::external::queue::enqueue::Request"};

                           common::log::line( verbose::log, "message: ", message);

                           // Send lookup
                           if( ! queue::lookup::send( state, message))
                           {
                              common::log::line( common::log::category::error, "failed to lookup queue - action: send error reply");

                              casual::queue::ipc::message::group::enqueue::Reply reply;
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
                        return [&state]( casual::queue::ipc::message::group::dequeue::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::handle::queue::dequeue::Request::operator()"};

                           common::log::line( verbose::log, "message: ", message);

                           // Send lookup
                           if( ! queue::lookup::send( state, message))
                           {
                              common::log::line( common::log::category::error, "failed to lookup queue - action: send error reply");

                              casual::queue::ipc::message::group::enqueue::Reply reply;
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
                  namespace discovery
                  {
                     auto request( State& state)
                     {
                        return []( gateway::message::domain::discovery::Request& message)
                        {
                           Trace trace{ "gateway::inbound::handle::local::external::domain::discovery::request"};
                           common::log::line( verbose::log, "message: ", message);

                           // we push it to the internal side, so it's possible to correlate the message.
                           // We don't know which external socket has sent this message at this moment.
                           // when we're done here, the 'external-dispatch' will correlate the socket descriptor
                           // and the correlation id of the message, so we can access it from the internal part.
                           ipc::inbound().push( std::move( message));                         
                        };
                     }
                  } // discovery
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
                           ipc::flush::send( ipc::manager::transaction(), message);
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

            // service
            local::internal::service::lookup::reply( state),
            local::internal::service::call::reply( state),

            // queue
            local::internal::queue::lookup::reply( state),
            local::internal::queue::dequeue::reply( state),
            local::internal::queue::enqueue::reply( state),

            // domain discovery
            local::internal::domain::discovery::request( state),
            local::internal::domain::discovery::reply( state),

            // transaction
            local::internal::transaction::resource::prepare::reply( state),
            local::internal::transaction::resource::commit::reply( state),
            local::internal::transaction::resource::rollback::reply( state),
         };
      }

      external_handler external( State& state)
      {
         return {
            
            // just a forward to the internal connect::request
            local::external::connect::request( state),
            local::external::disconnect::reply( state),

            // service call
            local::external::service::call::request( state),

            // queue
            local::external::queue::enqueue::request( state),
            local::external::queue::dequeue::request( state),

            // domain discover
            local::external::domain::discovery::request( state),

            // transaction
            local::external::transaction::resource::prepare::request(),
            local::external::transaction::resource::commit::request(),
            local::external::transaction::resource::rollback::request()
         };
      }


      namespace connection
      {
         std::optional< configuration::model::gateway::inbound::Connection> lost( State& state, common::strong::file::descriptor::id descriptor)
         {
            Trace trace{ "gateway::group::inbound::handle::connection::lost"};
            log::line( verbose::log, "descriptor: ", descriptor);

            auto result = state.external.remove( state.directive, descriptor);

            // find possible pending 'lookup' requests
            auto lost = algorithm::extract( state.correlations, algorithm::filter( state.correlations, predicate::value::equal( descriptor)));
            log::line( verbose::log, "lost: ", lost);

            auto pending = state.pending.requests.consume( algorithm::transform( lost, []( auto& lost){ return lost.correlation;}));
            
            // discard service lookup
            algorithm::for_each( pending.services, []( auto& call)
            {
               common::message::service::lookup::discard::Request request{ process::handle()};
               request.correlation = call.correlation;
               request.requested = call.service.name;
               // we don't need the reply
               request.reply = false;

               ipc::flush::optional::send( ipc::manager::service(), request);
            });

            algorithm::trim( state.pending.disconnects, algorithm::remove( state.pending.disconnects, descriptor));

            // There is no other pending we need to discard at the moment.

            return result;
         }

         void disconnect( State& state, common::strong::file::descriptor::id descriptor)
         {
            Trace trace{ "gateway::group::inbound::handle::connection::disconnect"};
            log::line( verbose::log, "descriptor: ", descriptor);

            if( auto connection = state.external.connection( descriptor))
            {
               log::line( verbose::log, "connection: ", *connection);

               if( connection->protocol == decltype( connection->protocol)::version_1_1)
               {
                  try 
                  {
                     state.correlations.emplace_back( 
                        connection->send( state, message::domain::disconnect::Request{}),
                        descriptor
                     );
                  }
                  catch( ...)
                  {
                     exception::sink::log();
                     handle::connection::lost( state, descriptor);
                  }
               }
               else
               {
                  handle::connection::lost( state, descriptor);
               }
            }
         }
         
      } // connection

      void idle( State& state)
      {
         // take care of pending disconnects
         if( ! state.pending.disconnects.empty())
         {
            auto connection_done = [&state]( auto descriptor)
            {
               return algorithm::find( state.correlations, descriptor).empty();
            };

            auto& pending = state.pending.disconnects;

            auto done = algorithm::extract( pending, algorithm::filter( pending, connection_done));

            for( auto descriptor : done)
               handle::connection::lost( state, descriptor);
         }
      }

      void shutdown( State& state)
      {
         Trace trace{ "gateway::group::inbound::handle::shutdown"};

         // try to do a 'soft' disconnect. copy - connection::disconnect mutates external
         for( auto descriptor : range::to_vector( state.external.descriptors))
            handle::connection::disconnect( state, descriptor);
      }

      void abort( State& state)
      {
         Trace trace{ "gateway::group::inbound::handle::abort"};

         state.runlevel = decltype( state.runlevel())::error;

         // 'kill' all sockets, and try to take care of pending stuff. copy - connection::lost mutates external
         for( auto descriptor : range::to_vector( state.external.descriptors))
            handle::connection::lost( state, descriptor);
      }
      

   } // gateway::group::inbound::handle

} // casual