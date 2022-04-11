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
#include "common/message/internal.h"
#include "common/event/listen.h"

#include "casual/assert.h"

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
                  if( auto connection = state.consume( message.correlation))
                  {
                     try
                     {
                        connection->send( state.directive, std::forward< M>( message));
                        return connection->descriptor();
                     }
                     catch( ...)
                     {
                        const auto error = exception::capture();

                        auto lost = connection::lost( state, connection->descriptor());

                        if( error.code() != code::casual::communication_unavailable)
                           log::line( log::category::error, error, " send failed to remote: ", lost.remote, " - action: remove connection");

                        // we 'lost' the connection in some way - we put a connection::Lost on our own ipc-device, and handle it
                        // later (and differently depending on if we're 'regular' or 'reversed')
                        communication::ipc::inbound::device().push( lost);
                     }
                     return {};
                  }
               
                  log::line( log::category::error, code::casual::communication_unavailable, " connection absent when trying to send reply - ", message.type());
                  log::line( log::category::verbose::error, "state: ", state);
               
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

               namespace service
               {
                  namespace lookup
                  {
                     namespace detail
                     {
                        template< typename R, typename E>
                        void reply( State& state, R&& request, common::message::service::lookup::Reply& lookup, E send_error)
                        {
                           
                           switch( lookup.state)
                           {
                              using Enum = decltype( lookup.state);
                              case Enum::idle:
                              {
                                 if( ! ipc::flush::optional::send( lookup.process.ipc, request))
                                 {
                                    log::line( common::log::category::error, common::code::xatmi::service_error, " server: ", lookup.process, " has been terminated during interdomain call - action: reply with: ", common::code::xatmi::service_error);
                                    send_error( state, lookup, common::code::xatmi::service_error);
                                 }
                                 break;
                              }
                              case Enum::absent:
                              {
                                 log::line( common::log::category::error, common::code::xatmi::no_entry, " service: ", lookup.service, " is not handled by this domain (any more) - action: reply with: ", common::code::xatmi::no_entry);
                                 send_error( state, lookup, common::code::xatmi::no_entry);
                                 break;
                              }
                              default:
                              {
                                 log::line( common::log::category::error, common::code::xatmi::system, " unexpected state on lookup reply: ", lookup, " - action: reply with: ", common::code::xatmi::service_error);
                                 send_error( state, lookup, common::code::xatmi::system);
                                 break;
                              }
                           }

                        }
                     } // detail

                     auto reply( State& state)
                     {
                        return [&state]( common::message::service::lookup::Reply& lookup)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::internal::call::lookup::reply"};
                           common::log::line( verbose::log, "message: ", lookup);

                           auto request = state.pending.requests.consume( lookup.correlation, lookup);

                           switch( common::message::type( request))
                           {
                              using Type = decltype( common::message::type( request));

                              case Type::service_call:

                                 detail::reply( state, request, lookup, []( auto& state, auto& lookup, auto code)
                                 {
                                    common::message::service::call::Reply reply;
                                    reply.correlation = lookup.correlation;
                                    reply.code.result = code;
                                    tcp::send( state, reply);
                                 });

                                 break;

                              case Type::conversation_connect_request:
                                 
                                 detail::reply( state, request, lookup, []( auto& state, auto& lookup, auto code)
                                 {
                                    common::message::conversation::connect::Reply reply;
                                    reply.correlation = lookup.correlation;
                                    reply.code.result = code;
                                    tcp::send( state, reply);
                                 });
                                 break;

                              default:
                                 log::line( log::category::error, code::casual::internal_unexpected_value, " message type: ", common::message::type( request), " - action: discard");

                           }

                        };  
                     }

                  } // lookup

                  namespace call
                  {
                     auto reply = basic_forward< common::message::service::call::Reply>;
                  } // call

               } // service

               namespace conversation
               {
                  namespace connect
                  {
                     auto reply( State& state)
                     {
                        return [&state]( common::message::conversation::connect::Reply& message)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::internal::conversation::connect::reply"};
                           common::log::line( verbose::log, "message: ", message);

                           // we consume the correlation to connection, and add a conversation specific "route"
                           if( auto descriptor = tcp::send( state, message))
                           {
                              state.conversations.push_back( state::Conversation{ message.correlation, descriptor, message.process});
                              common::log::line( verbose::log, "state.conversations: ", state.conversations);
                           }
                        };
                     }
                     
                  } // connect

                  auto send( State& state)
                  {
                     return [&state]( common::message::conversation::callee::Send& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::internal::conversation::send"};
                        common::log::line( verbose::log, "message: ", message);

                        if( auto found = algorithm::find( state.conversations, message.correlation))
                        {
                           if( auto connection = state.external.connection( found->descriptor))
                              connection->send( state.directive, message);
                           
                           // if the `send` has event that indicate that it will end the conversation - we remove our "route" state
                           // for the connection
                           if( message.code.result != decltype( message.code.result)::absent)
                              state.conversations.erase( std::begin( found));
                        }
                        else
                           common::log::line( common::log::category::error, "(internal) failed to correlate conversation: ", message.correlation);

                     };
                  }
                  
               } // conversation

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
                  auto connected( State& state)
                  {
                     return [&state]( const gateway::message::domain::Connected& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::internal::domain::connected"};
                        common::log::line( verbose::log, "message: ", message);

                        state.external.connected( state.directive, message);

                        common::log::line( verbose::log, "state.external: ", state.external);
       
                     };
                  }

                  namespace discovery
                  {
                     auto reply = basic_forward< casual::domain::message::discovery::Reply>;

                     namespace topology
                     {
                        auto update( State& state)
                        {
                           return [&state]( const casual::domain::message::discovery::topology::Update& message)
                           {
                              Trace trace{ "gateway::group::inbound::handle::local::internal::domain::discovery::topology::update"};
                              common::log::line( verbose::log, "message: ", message);

                              auto send_if_compatible = [ &state, &message]( auto& connection)
                              {
                                 if( ! message::protocol::compatible( message, connection.protocol()))
                                    return;

                                 auto information = state.external.information( connection.descriptor());
                                 casual::assertion( information, "information not valid for descriptor: ", connection.descriptor());

                                 if( information->configuration.discovery != decltype( information->configuration.discovery)::forward)
                                    return;

                                 // check if domain has seen this message before...
                                 if( algorithm::find( message.domains, information->domain))
                                    return;
                                 
                                 connection.send( state.directive, message);
                              };

                              algorithm::for_each( state.external.connections(), send_if_compatible);

                           };
                        }

                     } // topology

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
               template< typename M>
               auto correlate( State& state, const M& message)
               {
                  state.correlations.emplace_back( message.correlation, state.external.last());
                  return state.external.last();
               }

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
                  namespace detail
                  {
                     template< typename M>
                     auto lookup( State& state, M&& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::external::service::detail::lookup"};
                        common::log::line( verbose::log, "message: ", message);

                        // Change 'sender' so we get the reply
                        message.process = common::process::handle();

                        // Prepare lookup
                        common::message::service::lookup::Request request{ common::process::handle()};
                        {
                           request.correlation = message.correlation;
                           request.requested = message.service.name;
                           request.context = decltype( request.context)::no_busy_intermediate;
                        }

                        // Add message to pending
                        state.pending.requests.add( std::move( message));

                        // Send lookup
                        ipc::flush::send( ipc::manager::service(), request); 

                     }
                  } // detail
                  namespace call
                  {
                     auto request( State& state)
                     {
                        return [&state]( common::message::service::call::callee::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::external::service::call::request"};
                           log::line( verbose::log, "message: ", message);
                           
                           external::correlate( state, message);
                           
                           detail::lookup( state, message);
                        };
                     }
                  } // call
               } // service

               namespace conversation
               {
                  namespace connect
                  {
                     auto request( State& state)
                     {
                        return [&state]( common::message::conversation::connect::callee::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::external::conversation::connect::request"};
                           log::line( verbose::log, "message: ", message);
                           
                           external::correlate( state, message);

                           service::detail::lookup( state, message);
                        };
                     }
                     
                  } // connect

                  auto disconnect( State& state)
                  {
                     return [&state]( common::message::conversation::Disconnect& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::external::conversation::disconnect"};
                        common::log::line( verbose::log, "message: ", message);

                        if( auto found = algorithm::find( state.conversations, message.correlation))
                        {
                           ipc::flush::send( found->process.ipc, message);

                           // we're done with this connection, regardless of what callee thinks...
                           state.conversations.erase( std::begin( found));
                        }
                     };
                  }

                  auto send( State& state)
                  {
                     return [&state]( common::message::conversation::callee::Send& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::external::conversation::send"};
                        common::log::line( verbose::log, "message: ", message);

                        if( auto found = algorithm::find( state.conversations, message.correlation))
                           ipc::flush::send( found->process.ipc, message);
                        else
                           common::log::line( common::log::category::error, "(external) failed to correlate conversation: ", message.correlation);
                     };
                  }
                  
               } // conversation


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

                           external::correlate( state, message);

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

                           external::correlate( state, message);

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
                        return [&state]( casual::domain::message::discovery::Request& message)
                        {
                           Trace trace{ "gateway::inbound::handle::local::external::domain::discovery::request"};
                           common::log::line( verbose::log, "message: ", message);

                           auto descriptor = external::correlate( state, message);

                           // Set 'sender' so we get the reply
                           message.process = common::process::handle();

                           auto information = state.external.information( descriptor);
                           assert( information);

                           if( information->configuration.discovery == decltype( information->configuration.discovery)::forward)
                              message.directive = decltype( message.directive)::forward;

                           casual::domain::discovery::request( message);                    
                        };
                     }
                  } // discovery
               } // domain
         
               namespace transaction
               {
                  namespace resource
                  {
                     template< typename Message>
                     auto basic_request( State& state)
                     {
                        return [&state]( Message& message)
                        {
                           Trace trace{ "gateway::inbound::handle::local::external::transaction::basic_request"};
                           common::log::line( verbose::log, "message: ", message);

                           external::correlate( state, message);

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
         casual::domain::discovery::provider::registration( casual::domain::discovery::provider::Ability::topology);

         return {
            common::message::handle::defaults( communication::ipc::inbound::device()),
            common::message::internal::dump::state::handle( state),

            // lookup
            local::internal::service::lookup::reply( state),
            
            // service
            local::internal::service::call::reply( state),

            // conversation
            local::internal::conversation::connect::reply( state),
            local::internal::conversation::send( state),

            // queue
            local::internal::queue::lookup::reply( state),
            local::internal::queue::dequeue::reply( state),
            local::internal::queue::enqueue::reply( state),

            // domain discovery
            local::internal::domain::discovery::reply( state),
            local::internal::domain::discovery::topology::update( state),

            // transaction
            local::internal::transaction::resource::prepare::reply( state),
            local::internal::transaction::resource::commit::reply( state),
            local::internal::transaction::resource::rollback::reply( state),

            local::internal::domain::connected( state),
         };
      }

      external_handler external( State& state)
      {
         return {
            
            local::external::disconnect::reply( state),

            // service call
            local::external::service::call::request( state),

            // conversation
            local::external::conversation::connect::request( state),
            local::external::conversation::disconnect( state),
            local::external::conversation::send( state),

            // queue
            local::external::queue::enqueue::request( state),
            local::external::queue::dequeue::request( state),

            // discover
            local::external::domain::discovery::request( state),

            // transaction
            local::external::transaction::resource::prepare::request( state),
            local::external::transaction::resource::commit::request( state),
            local::external::transaction::resource::rollback::request( state)
         };
      }


      namespace connection
      {
         message::inbound::connection::Lost lost( State& state, common::strong::file::descriptor::id descriptor)
         {
            Trace trace{ "gateway::group::inbound::handle::connection::lost"};
            log::line( verbose::log, "descriptor: ", descriptor);

            auto information = state.external.remove( state.directive, descriptor);

            // find possible pending 'lookup' requests
            auto lost = algorithm::container::extract( state.correlations, algorithm::filter( state.correlations, predicate::value::equal( descriptor)));
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

            algorithm::container::trim( state.pending.disconnects, algorithm::remove( state.pending.disconnects, descriptor));

            // There is no other pending we need to discard at the moment.

            return { std::move( information.configuration), std::move( information.domain)};
         }

         void disconnect( State& state, common::strong::file::descriptor::id descriptor)
         {
            Trace trace{ "gateway::group::inbound::handle::connection::disconnect"};
            log::line( verbose::log, "descriptor: ", descriptor);

            if( auto connection = state.external.connection( descriptor))
            {
               log::line( verbose::log, "connection: ", *connection);

               if( message::protocol::compatible< message::domain::disconnect::Request>( connection->protocol()))
               {
                  try 
                  {
                     state.correlations.emplace_back( 
                        connection->send( state.directive, message::domain::disconnect::Request{}),
                        descriptor
                     );
                  }
                  catch( ...)
                  {
                     exception::sink();
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
         Trace trace{ "gateway::group::inbound::handle::idle"};

         // take care of pending disconnects
         if( ! state.pending.disconnects.empty())
         {
            auto connection_done = [&state]( auto descriptor)
            {
               return algorithm::find( state.correlations, descriptor).empty();
            };

            auto& pending = state.pending.disconnects;

            auto done = algorithm::container::extract( pending, algorithm::filter( pending, connection_done));

            for( auto descriptor : done)
               handle::connection::lost( state, descriptor);
         }
      }

      void shutdown( State& state)
      {
         Trace trace{ "gateway::group::inbound::handle::shutdown"};

         state.runlevel = decltype( state.runlevel())::shutdown;

         // try to do a 'soft' disconnect. copy - connection::disconnect mutates external
         for( auto descriptor : state.external.descriptors())
            handle::connection::disconnect( state, descriptor);
      }

      void abort( State& state)
      {
         Trace trace{ "gateway::group::inbound::handle::abort"};

         state.runlevel = decltype( state.runlevel())::error;

         // 'kill' all sockets, and try to take care of pending stuff. copy - connection::lost mutates external
         for( auto descriptor : state.external.descriptors())
            handle::connection::lost( state, descriptor);
      }
      

   } // gateway::group::inbound::handle

} // casual