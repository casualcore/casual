//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/outbound/handle.h"
#include "gateway/group/tcp.h"
#include "gateway/group/ipc.h"

#include "gateway/message.h"
#include "gateway/message/protocol.h"
#include "gateway/message/protocol/transform.h"
#include "gateway/common.h"

#include "domain/discovery/api.h"

#include "common/communication/instance.h"
#include "common/message/dispatch/handle.h"
#include "common/message/internal.h"
#include "common/event/send.h"
#include "common/event/listen.h"
#include "common/instance.h"

namespace casual
{
   using namespace common;

   namespace gateway::group::outbound::handle
   {
      namespace local
      {
         namespace
         {
            namespace tcp
            {
               template< typename M>
               strong::correlation::id send( State& state, strong::socket::id descriptor, M&& message)
               {
                  return group::tcp::send( state, &connection::lost, descriptor, std::forward< M>( message));
               }
            } // tcp

            namespace internal
            {
               namespace precondition
               {
                  bool reply( State& state, strong::socket::id descriptor, auto& message, auto&& create_reply)
                  {
                     if( state.runlevel > decltype( state.runlevel())::running)
                     {
                        log::line( log, "outbound is in shutdown mode - action: reply with 'default'");
                        state.multiplex.send( message.process.ipc, create_reply( message));
                        return true;
                     }

                     if( algorithm::find( state.disconnecting, descriptor))
                     {
                        log::line( log, "connection: ", descriptor, " is in disconnect mode - action: reply with 'default'");
                        state.multiplex.send( message.process.ipc, create_reply( message));
                        return true;
                     }

                     return false;
                  }
               } // precondition

               namespace transaction
               {
                  template< typename M> 
                  void associate_and_involve( State& state, const M& message, strong::socket::id descriptor)
                  {
                     Trace trace{ "gateway::group::outbound::handle::local::internal::transaction::associate_and_involve"};
                     log::line( verbose::log, "message: ", message);

                     if( message.trid)
                     {
                        if( ! state.pending.transactions.associate( message.trid, descriptor))
                           return;

                        // We can't really get rid of this (now), We need to make sure TM get's the involve message
                        // before we do anything else. Otherwise the transaction might get committed and TM does not know
                        // about this _involved external resource_.
                        if( auto handle = state.connections.process_handle( descriptor))
                        {
                           ipc::flush::send( 
                              ipc::manager::transaction(),
                              common::message::transaction::resource::external::involved::create( message, handle));
                        }
                        else
                           log::error( code::casual::invalid_semantics, "failed to find the ipc partner to ", descriptor);
                     }
                  }

                  namespace resource
                  {  
                     namespace basic
                     {
                        template< typename Message>
                        auto request( State& state)
                        {
                           return [ &state]( Message& message, strong::ipc::descriptor::id descriptor)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::transaction::resource::basic_request"};
                              log::line( verbose::log, "message: ", message);

                              auto tcp = state.connections.partner( descriptor);

                              // sanity check
                              if( ! state.pending.transactions.is_associated( message.trid, tcp))
                                 log::error( code::casual::invalid_semantics, message.type(), " with trid: ", message.trid, " sent to ", tcp, " is not associated before");

                              // add the destination, so we know where to send the reply
                              state.reply_destination.add( message, tcp);

                              tcp::send( state, tcp, message);
                           };
                        }
                     } // basic

                     //! These messages has the extern branched transaction.
                     //! @{
                     namespace prepare
                     {
                        auto request = basic::request< common::message::transaction::resource::prepare::Request>;
                     } // prepare
                     namespace commit
                     {
                        auto request = basic::request< common::message::transaction::resource::commit::Request>;
                     } // commit
                     namespace rollback
                     {
                        auto request = basic::request< common::message::transaction::resource::rollback::Request>;
                     } // rollback
                  } // resource
               } // transaction

               namespace service
               {
                  void unadvertise( State& state, strong::socket::id descriptor, std::vector< std::string> services)
                  {
                     if( auto handle = state.connections.process_handle( descriptor))
                     {
                        common::message::service::Advertise unadvertise{ handle};
                        unadvertise.alias = instance::alias();
                        unadvertise.services.remove = std::move( services);
                        state.multiplex.send( ipc::manager::service(), unadvertise);
                     }
                     else
                        log::error( code::casual::invalid_semantics, "failed to unadvertise - could not find ipc partner to ", descriptor);
                  }

                  template< typename M, typename D>
                  auto metric( State& state, const M& message, const common::transaction::ID& trid, D&& destination, code::xatmi code)
                  {
                     Trace trace{ "gateway::group::outbound::handle::local::internal::service::call::metric"};

                     common::message::event::service::Metric metric;
                     {   
                        metric.process = common::process::handle();
                        metric.correlation = message.correlation;
                        metric.execution = message.execution;
                        metric.service = std::move( destination.service);
                        metric.parent = std::move( destination.parent);
                        metric.type = decltype( metric.type)::concurrent;
                        metric.code = code;
                        
                        metric.trid = trid;
                        metric.start = destination.start;
                        metric.end = platform::time::clock::type::now();
                     }
                     state.service_metric.add( std::move( metric));
                     state.service_metric.maybe_metric( state, &handle::metric::send);
                  }

                  namespace call
                  {
                     namespace detail::send::error
                     {
                        void reply( State& state, const common::message::service::call::callee::Request& message, code::xatmi code)
                        {
                           auto reply = common::message::reverse::type( message);
                           reply.code.result = code;
                           reply.transaction.trid = message.trid;
                           
                           struct Destination
                           {
                              std::string service;
                              execution::context::Parent parent;
                              platform::time::point::type start;
                           };

                           state.multiplex.send( message.process.ipc, reply);

                           service::metric( state, reply, message.trid, Destination{ message.service.name, message.parent, platform::time::clock::type::now()}, code);

                        }
                        
                     } // detail::send::error   

                     namespace detail::create
                     {
                        auto task( State& state, const common::message::service::call::callee::Request& message, strong::socket::id descriptor)
                        {
                           struct Destination
                           {
                              std::string service;
                              execution::context::Parent parent;
                              strong::ipc::id ipc;
                              common::transaction::ID trid;
                              platform::time::point::type start;
                           };

                           auto shared = std::make_shared< Destination>( Destination{ message.service.name, message.parent, message.process.ipc, message.trid, platform::time::clock::type::now()});
                           
                           return typename state::task_coordinator_type::unit_type{ descriptor, message.correlation, 
                           [ &state, shared]( common::message::service::call::Reply& message, strong::socket::id descriptor)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::detail::create::task call::Reply"};
                              log::line( verbose::log, "message: ", message);

                              state.multiplex.send( shared->ipc, message);

                              // we unadvertise the service if we get no_entry
                              if( message.code.result == decltype( message.code.result)::no_entry)
                                 service::unadvertise( state, descriptor, { shared->service});

                              internal::service::metric( state, message, message.transaction.trid, std::move( *shared), message.code.result);
                           },
                           [ &state, shared]( casual::task::concurrent::message::task::Failed& message, strong::socket::id descriptor)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::detail::create::task task::Failed"};

                              common::message::service::call::Reply reply;
                              reply.correlation = message.correlation;
                              reply.code.result = common::code::xatmi::service_error;
                              reply.transaction.trid = shared->trid;

                              state.multiplex.send( shared->ipc, reply);

                              service::metric( state, reply, shared->trid, std::move( *shared), reply.code.result);
                           }};
                        }
                        
                     } // detail::create

                     auto request( State& state)
                     {
                        return [ &state]( common::message::service::call::callee::Request& message, strong::ipc::descriptor::id descriptor)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::internal::service::call::request"};
                           log::line( verbose::log, "message: ", message);

                           // Check if we've has been called with the same correlation id before, 
                           // hence we are in a loop between gateways.
                           if( state.tasks.contains( message.correlation))
                           {
                              log::line( log::category::error, code::casual::invalid_semantics, " a call with the same correlation id is in flight - ", message.correlation, " - action: reply with ", code::xatmi::system   );
                              detail::send::error::reply( state, message, code::xatmi::system);
                              return;
                           }

                           auto connection = state.connections.find_external( descriptor);
                           CASUAL_ASSERT( connection);

                           if( message::protocol::compatible< common::message::service::call::callee::Request>( connection->protocol()))
                           {
                              tcp::send( state, connection->descriptor(), message);

                              state.tasks.add( detail::create::task( state, message, connection->descriptor()));
                              transaction::associate_and_involve( state, message, connection->descriptor());
                           }
                           else
                           {
                              // we need to do this first, since we're doing a destructive transform on the message (we cant use after moved)
                              state.tasks.add( detail::create::task( state, message, connection->descriptor()));
                              transaction::associate_and_involve( state, message, connection->descriptor());

                              tcp::send( state, connection->descriptor(), message::protocol::transform::to< common::message::service::call::v1_2::callee::Request>( std::move( message)));
                           }

                        };
                     }

                  } // call
               } // service

               namespace conversation
               {
                  namespace connect
                  {
                     namespace detail::create
                     {
                        auto task( State& state, common::message::conversation::connect::callee::Request& message, strong::socket::id descriptor)
                        {
                           struct Shared
                           {
                              std::string service;
                              execution::context::Parent parent;
                              strong::ipc::id ipc;
                              platform::time::point::type start;
                              common::transaction::ID trid;

                           };

                           auto shared = std::make_shared< Shared>( Shared{ message.service.name, message.parent, message.process.ipc, platform::time::clock::type::now(), message.trid});


                           return typename state::task_coordinator_type::unit_type{ descriptor, message.correlation, 
                              [ &state, shared]( common::message::conversation::connect::Reply& message, strong::socket::id descriptor)
                              {
                                 Trace trace{ "gateway::group::outbound::handle::local::internal::conversation::task connect::Reply"};

                                 // we unadvertise the service if we get no_entry, and we got 
                                 // no connections left for the service
                                 if( message.code.result == decltype( message.code.result)::no_entry)
                                    service::unadvertise( state, descriptor, { shared->service});

                                 state.multiplex.send( shared->ipc, message);

                                 return task::concurrent::unit::Dispatch::pending;
                              },
                              [ &state, shared]( common::message::conversation::callee::Send& message, strong::socket::id descriptor)
                              {
                                 Trace trace{ "gateway::group::outbound::handle::local::internal::conversation::task Send"};

                                 state.multiplex.send( shared->ipc, message);

                                 // if the send indicate a termination, we make sure to remove this task
                                 if( message.duplex == decltype( message.duplex)::terminated)
                                    return task::concurrent::unit::Dispatch::done;

                                 return task::concurrent::unit::Dispatch::pending;
                              },
                              [ &state, shared]( common::message::conversation::Disconnect& message, strong::socket::id descriptor)
                              {
                                 Trace trace{ "gateway::group::outbound::handle::local::internal::conversation::task Disconnect"};

                                 // this will be invoked from internal side. Only the one who connect, can disconnect.
                                 // We'll be done with the task, and send metric to SM

                                 tcp::send( state, descriptor, message);

                                 service::metric( state, message, shared->trid, *shared, code::xatmi::ok);

                                 return task::concurrent::unit::Dispatch::done;
                              }
                              
                           };
                        }

                     } // detail::create

                     auto request( State& state)
                     {
                        return [ &state]( common::message::conversation::connect::callee::Request& message, strong::ipc::descriptor::id descriptor)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::internal::conversation::connect::request"};
                           log::line( verbose::log, "message: ", message);

                           auto connection = state.connections.find_external( descriptor);
                           CASUAL_ASSERT( connection);

                           state.tasks.add( detail::create::task( state, message, connection->descriptor()));
                           transaction::associate_and_involve( state, message, connection->descriptor());

                           if( message::protocol::compatible< common::message::conversation::connect::callee::Request>( connection->protocol()))
                              tcp::send( state, connection->descriptor(), message);
                           else
                              tcp::send( state, connection->descriptor(), message::protocol::transform::to< common::message::conversation::connect::v1_2::callee::Request>( std::move( message)));

                        };
                     }

                  } // connect


                  auto disconnect( State& state)
                  {
                     return [&state]( common::message::conversation::Disconnect& message, strong::ipc::descriptor::id descriptor)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::internal::conversation::disconnect"};
                        log::line( verbose::log, "message: ", message);

                        // this should en the task
                        state.tasks( message);
                     };
                  }

                  auto send( State& state)
                  {
                     return [&state]( common::message::conversation::callee::Send& message, strong::ipc::descriptor::id descriptor)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::internal::conversation::send"};
                        log::line( verbose::log, "message: ", message);

                        auto tcp = state.connections.partner( descriptor);
                        tcp::send( state, tcp, message);
                     };
                  }

               } // conversation

               namespace domain
               {
                  namespace discovery
                  {
                     auto request( State& state)
                     {
                        return [&state]( casual::domain::message::discovery::Request& message, strong::ipc::descriptor::id descriptor)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::internal::domain::discover::request"};
                           log::line( verbose::log, "message: ", message);

                           auto default_reply = []( auto& message){ return common::message::reverse::type( message);};

                           auto tcp = state.connections.partner( descriptor);

                           if( internal::precondition::reply( state, tcp, message, default_reply))
                              return;

                           state.reply_destination.add( message, tcp);

                           tcp::send( state, tcp, message);
                        };
                     }

                     namespace topology::direct
                     {

                        auto explore( State& state)
                        {
                           //! Sent from _discovery_ when:
                           //! * there are new connections (either from our process, and/or some other "outbound")
                           //! * AND/OR there are implicit topology updates (either from our process, and/or some other "outbound")
                           //! * after some specific time, _discovery_ gathers "known", and send topology::direct::Explore to us (and other "outbounds")
                           return [ &state]( casual::domain::message::discovery::topology::direct::Explore& message, strong::ipc::descriptor::id descriptor)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::domain::discover::topology::direct::explore"};
                              log::line( verbose::log, "message: ", message);

                              auto tcp = state.connections.partner( descriptor);

                              if( state.runlevel > decltype( state.runlevel())::running)
                                 return;

                              if( algorithm::find( state.disconnecting, tcp))
                                 return;

                              casual::domain::message::discovery::Request request;
                              request.content = std::move( message.content);
                              request.domain = common::domain::identity();
                              tcp::send( state, tcp, request);
                           
                              // note that we don't keep track of any reply destinations, since caller does not expect any.
                           };

                        }
                     } // topology::direct

                  } // discovery

               } // domain

               namespace queue
               {
                  namespace detail::create
                  {
                     auto task( State& state, auto& message, strong::socket::id descriptor)
                     {
                        using reply_type = common::message::reverse::type_t< decltype( message)>;
                     
                        return typename state::task_coordinator_type::unit_type{ descriptor, message.correlation, 
                        [ &state, ipc = message.process.ipc]( reply_type& message, strong::socket::id descriptor)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::internal::queue::detail::create::task Reply"};
                           log::line( verbose::log, "message: ", message);

                           state.multiplex.send( ipc, message);
                        },
                        [ &state, ipc = message.process.ipc]( casual::task::concurrent::message::task::Failed& message, strong::socket::id descriptor)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::internal::detail::create::task task::Failed"};

                           reply_type reply;
                           reply.correlation = message.correlation;

                           state.multiplex.send( ipc, reply);
                        }};
                     }
                     
                  } // detail::create

                  namespace basic
                  {
                     template< typename Message>
                     auto request( State& state)
                     {
                        return [&state]( Message& message, strong::ipc::descriptor::id descriptor)
                        {
                           Trace trace{ "gateway::outbound::local::handle::internal::queue::basic::request"};
                           log::line( verbose::log, "message: ", message);

                           auto tcp = state.connections.partner( descriptor);

                           tcp::send( state, tcp, message);
                           state.tasks.add( detail::create::task( state, message, tcp));

                           transaction::associate_and_involve( state, message, tcp);
                        };
                     }  
                  } // basic

                  namespace enqueue
                  {
                     auto request = basic::request< casual::queue::ipc::message::group::enqueue::Request>;
                  } // enqueue

                  namespace dequeue
                  {
                     auto request = basic::request< casual::queue::ipc::message::group::dequeue::Request>;
                  } // dequeue
               } // queue
               
            } // internal

            namespace external
            {
               template< typename Message>
               auto basic_task( State& state)
               {
                  return [ &state]( Message& message)
                  {
                     Trace trace{ "gateway::group::outbound::handle::local::external::basic_task"};
                     common::log::line( verbose::log, "message: ", message);

                     state.tasks( message);
                     common::log::line( verbose::log, "state.tasks: ", state.tasks);
                  };
               }
               

               namespace disconnect
               {
                  auto request( State& state)
                  {
                     return [&state]( const gateway::message::domain::disconnect::Request& message, strong::socket::id descriptor)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::external::disconnect::request"};
                        log::line( verbose::log, "message: ", message);

                        handle::connection::disconnect( state, descriptor);
                        tcp::send( state, descriptor, common::message::reverse::type( message));
                     };
                  }
                  
               } // disconnect

               namespace service
               {
                  namespace call
                  {
                     auto reply = basic_task< common::message::service::call::Reply>;

                     namespace v1_2
                     {
                        auto reply( State& state)
                        {
                           return [ &state]( common::message::service::call::v1_2::Reply message)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::external::service::call::v1_2::reply"};
                              log::line( verbose::log, "message: ", message);

                              state.tasks( message::protocol::transform::from( std::move( message)));
                           };
                        }
                     } // v1_2
                  } // call

               } // service

               namespace conversation
               {
                  namespace connect
                  {
                     auto reply = basic_task< common::message::conversation::connect::Reply>;

                  } // connect

                  auto send = basic_task< common::message::conversation::callee::Send>;

               } // conversation

               namespace queue
               {
                  namespace enqueue
                  {
                     auto reply = basic_task< casual::queue::ipc::message::group::enqueue::Reply>;

                     namespace v1_2
                     {
                        auto reply( State& state)
                        {
                           return [ &state]( casual::queue::ipc::message::group::enqueue::v1_2::Reply& message, strong::socket::id descriptor)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::external::queue::enqueue::v1_2::reply"};
                              log::line( verbose::log, "message: ", message);

                              state.tasks( message::protocol::transform::from( std::move( message)));
                           };
                        }   
                     } // v1_2

                  } // enqueue

                  namespace dequeue
                  {
                     auto reply = basic_task< casual::queue::ipc::message::group::dequeue::Reply>;

                     namespace v1_2
                     {
                        auto reply( State& state)
                        {
                           return [ &state]( casual::queue::ipc::message::group::dequeue::v1_2::Reply& message, strong::socket::id descriptor)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::external::queue::dequeue::v1_2::reply"};
                              log::line( verbose::log, "message: ", message);

                              state.tasks( message::protocol::transform::from( std::move( message)));
                           };
                        }   
                     } // v1_2

                  } // dequeue
               } // queue

               namespace transaction::resource
               {
                  namespace detail::basic
                  {
                     template< typename Message>
                     auto reply( State& state)
                     {
                        return [ &state]( Message& message, strong::socket::id descriptor)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::external::transaction::resource::detail::basic_reply"};
                           log::line( verbose::log, "message: ", message);

                           auto destination = state.reply_destination.extract( message.correlation);
                           state.multiplex.send( destination.ipc, message);
                        };

                     }                     
                  } // detail::basic

                  namespace prepare
                  {
                     auto reply = detail::basic::reply< common::message::transaction::resource::prepare::Reply>;

                  } // prepare

                  namespace commit
                  {
                     auto reply = detail::basic::reply< common::message::transaction::resource::commit::Reply>;

                  } // commit

                  namespace rollback
                  {
                     auto reply = detail::basic::reply< common::message::transaction::resource::rollback::Reply>;
                     
                  } // rollback

               } // transaction::resource

               namespace domain::discovery
               {
                  auto reply( State& state)
                  {
                     return [ &state]( casual::domain::message::discovery::Reply&& message, strong::socket::id descriptor)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::external::domain::discover::reply"};
                        log::line( verbose::log, "message: ", message);

                        auto handle = state.connections.process_handle( descriptor);
                        CASUAL_ASSERT( handle);

                        auto information = state.connections.information( descriptor);
                        CASUAL_ASSERT( information);

                        if( ! message.content.services.empty())
                        {
                           // increase hops for all services.
                           for( auto& service : message.content.services)
                              ++service.property.hops;

                           common::message::service::concurrent::Advertise request{ handle};
                           request.alias = instance::alias();
                           request.description = information->domain.name;
                           request.order = state.order;
                           request.services.add = message.content.services;
                           
                           state.multiplex.send( ipc::manager::service(), request);
                        }

                        if( ! message.content.queues.empty())
                        {
                           casual::queue::ipc::message::Advertise request{ handle};
                           request.alias = instance::alias();
                           request.description = information->domain.name;
                           request.order = state.order;
                           request.queues.add = algorithm::transform( message.content.queues, []( auto& queue)
                           {
                              return casual::queue::ipc::message::advertise::Queue{ queue.name, queue.retries};
                           });

                           state.multiplex.send( ipc::manager::optional::queue(), request);
                        }

                        if( auto destination = state.reply_destination.extract( message.correlation))
                           state.multiplex.send( destination.ipc, message);

                        // if we don't have a destination, we assume the reply was due to topology::direct::Explore                        
      
                     };
                  }

                  namespace topology
                  {
                     //! we get this from inbounds that are configured with _discovery forward_ and the domain topology has 
                     //! been updated.
                     auto update( State& state)
                     {
                        return [ &state]( casual::domain::message::discovery::topology::implicit::Update&& message, strong::socket::id descriptor)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::external::domain::discover::topology::update"};
                           log::line( verbose::log, "message: ", message);

                           // no need to send it if we've seen this message before
                           if( algorithm::find( message.domains, common::domain::identity()))
                              return;

                           // set the actual correlated process/ipc
                           message.process = state.connections.process_handle( descriptor);
                           casual::domain::discovery::topology::implicit::update( state.multiplex, message);
                        };
                     }
                  } // topology

               } // domain::discovery
            } // external

            namespace management
            {
               namespace event::transaction
               {
                  auto disassociate( State& state)
                  {
                     return [ &state]( const common::message::event::transaction::Disassociate& message)
                     {
                        Trace trace{ "gateway::outbound::handle:::event::transaction::disassociate"};
                        log::line( verbose::log, "message: ", message);

                        state.pending.transactions.remove( message.gtrid.range());
                     };
                  }
               } // event::transaction

               namespace domain
               {
                  auto connected( State& state)
                  {
                     return [ &state]( const gateway::message::domain::Connected& message)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::internal::domain::connected"};
                        common::log::line( verbose::log, "message: ", message);

                        auto descriptors = state.connections.connected( state.directive, message);

                        auto inbound = state.connections.find_internal( descriptors.ipc);
                        CASUAL_ASSERT( inbound);

                        // a new connection has been established, we need to register this with discovery.
                        casual::domain::discovery::provider::registration( *inbound, casual::domain::discovery::provider::Ability::discover);

                        auto information = state.connections.information( descriptors.tcp);
                        CASUAL_ASSERT( information);

                        auto handle = state.connections.process_handle( descriptors.ipc);

                        // We let TM know about the new external resource, that might be used in the future
                        {
                           common::message::transaction::resource::external::Instance instance{ handle};
                           instance.alias = instance::alias();
                           instance.description = information->domain.name;
                           state.multiplex.send( ipc::manager::transaction(), instance);
                        }

                        // let the _discovery_ know that the topology has been updated
                        {
                           casual::domain::message::discovery::topology::direct::Update update{ handle};
                           
                           // should we supply the configured stuff.
                           if( information->configuration)
                           {
                              update.configured.services = information->configuration.services;
                              update.configured.queues = information->configuration.queues;
                           }
                           casual::domain::discovery::topology::direct::update( state.multiplex, update);
                        }
                     };
                  }

               } // domain
               
            } // management

         } // <unnamed>
      } // local

      management_handler management( State& state)
      {
         return management_handler{
            common::event::listener( 
               handle::local::management::event::transaction::disassociate( state)
            ),
            local::management::domain::connected( state)
         };

      }
         

      internal_handler internal( State& state)
      {   
         return internal_handler{


            // service
            local::internal::service::call::request( state),

            // conversation
            local::internal::conversation::connect::request( state),
            local::internal::conversation::disconnect( state),
            local::internal::conversation::send( state),
            
            // queue
            local::internal::queue::dequeue::request( state),
            local::internal::queue::enqueue::request( state),

            // transaction
            local::internal::transaction::resource::prepare::request( state),
            local::internal::transaction::resource::commit::request( state),
            local::internal::transaction::resource::rollback::request( state),

            // discover
            local::internal::domain::discovery::request( state),
            local::internal::domain::discovery::topology::direct::explore( state),
         };
      }

      external_handler external( State& state)
      {
         return external_handler{
            local::external::disconnect::request( state),
            
            // service
            local::external::service::call::reply( state),
            local::external::service::call::v1_2::reply( state),

            // conversation
            local::external::conversation::connect::reply( state),
            local::external::conversation::send( state),

            // queue
            local::external::queue::enqueue::reply( state),
            local::external::queue::enqueue::v1_2::reply( state),
            local::external::queue::dequeue::reply( state),
            local::external::queue::dequeue::v1_2::reply( state),

            // transaction
            local::external::transaction::resource::prepare::reply( state),
            local::external::transaction::resource::commit::reply( state),
            local::external::transaction::resource::rollback::reply( state),

            // discover
            local::external::domain::discovery::reply( state),
            local::external::domain::discovery::topology::update( state)
         };
      }

      void unadvertise( State& state, strong::socket::id descriptor)
      {
         Trace trace{ "gateway::group::outbound::handle::unadvertise"};
         log::line( verbose::log, "descriptor: ", descriptor);

         auto handle = state.connections.process_handle( descriptor);

         if( ! handle)
         {
            log::error( code::casual::internal_correlation, "failed to find ipc for ", descriptor);
            return;
         }

         {
            common::message::service::concurrent::Advertise request{ handle};
            request.alias = instance::alias();
            request.reset = true;
            state.multiplex.send( ipc::manager::service(), request);
         }

         {
            casual::queue::ipc::message::Advertise request{ handle};
            request.reset = true;
            if( ! state.multiplex.send( ipc::manager::optional::queue(), request))
               log::line( verbose::log, "queue-manager is not on-line");
         }
      }

      namespace connection
      {
         message::outbound::connection::Lost lost( State& state, strong::socket::id descriptor)
         {
            Trace trace{ "gateway::group::outbound::handle::connection::lost"};
            log::line( verbose::log, "descriptor: ", descriptor);

            // might not be necessary since we send an ipc-destroyed event below.
            handle::unadvertise( state, descriptor);

            // we need to send an ipc-destroyed event, so other can disassociate stuff with the ipc
            if( auto handle = state.connections.process_handle( descriptor))
               common::event::send( common::message::event::ipc::Destroyed{ handle});

            state.tasks.failed( descriptor);

            // extract all state associated with the descriptor
            auto extracted = state.failed( descriptor);
            log::line( verbose::log, "extracted: ", extracted);

            return { std::move( extracted.information.configuration), std::move( extracted.information.domain)};
         }

         void disconnect( State& state, common::strong::socket::id descriptor)
         {
            Trace trace{ "gateway::group::outbound::handle::connection::disconnect"};
            log::line( verbose::log, "descriptor: ", descriptor);

            // unadvertise all associated services and queues, if any.
            handle::unadvertise( state, descriptor);

            state.disconnecting.push_back( descriptor);
         }
         
      } // connection

      void idle( State& state)
      {
         Trace trace{ "gateway::group::outbound::handle::idle"};

         // we need to check metric, we don't know when we're about to be called again.
         state.service_metric.force_metric( state, &handle::metric::send);
      }

      void shutdown( State& state)
      {
         Trace trace{ "gateway::group::outbound::handle::shutdown"};

         state.runlevel = state::Runlevel::shutdown;

         for( auto descriptor : state.connections.external_descriptors())
            handle::connection::disconnect( state, descriptor);

         // send metric for good measure
         state.service_metric.force_metric( state, &handle::metric::send);

         log::line( verbose::log, "state: ", state);
      }

      void abort( State& state)
      {
         Trace trace{ "gateway::group::outbound::handle::abort"};
         log::line( log::category::verbose::error, "abort - state: ", state);

         state.runlevel = state::Runlevel::error;

         for( auto descriptor : state.connections.external_descriptors())
            handle::unadvertise( state, descriptor);

         state.connections.clear( state.directive);
      }

      namespace metric
      {
         void send( State& state, const common::message::event::service::Calls& metric)
         {
            Trace trace{ "gateway::group::outbound::handle::metric::send"};
            log::line( verbose::log, "metric: ", metric);

            state.multiplex.send( ipc::manager::service(), metric);
         }
      } // metric

   } // gateway::group::outbound::handle
} // casual