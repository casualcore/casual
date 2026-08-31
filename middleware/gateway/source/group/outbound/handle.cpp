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
                  return group::tcp::send( state, descriptor, std::forward< M>( message));
               }

            } // tcp

            namespace internal
            {
               template< typename Message>
               auto basic_task( State& state)
               {
                  return [ &state]( Message& message)
                  {
                     Trace trace{ "gateway::group::outbound::handle::local::internal::basic_task"};
                     common::log::debug( "message: ", message);

                     state.tasks( message);
                     common::log::debug( "state.tasks: ", state.tasks);
                  };
               }

               namespace precondition
               {
                  bool reply( State& state, strong::socket::id descriptor, auto& message, auto&& create_reply)
                  {
                     if( state.runlevel > decltype( state.runlevel())::running)
                     {
                        log::debug( "outbound is in shutdown mode - action: reply with 'default'");
                        state.multiplex.send( message.process.ipc, create_reply( message));
                        return true;
                     }

                     if( state.pending.dissociating.contains( descriptor))
                     {
                        log::debug( "connection: ", descriptor, " is in disconnect mode - action: reply with 'default'");
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
                     log::debug( "message: ", message);

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
                     namespace detail::create
                     {
                        auto task( State& state, auto& message, strong::socket::id descriptor)
                        {
                           using reply_type = common::message::reverse::type_t< decltype( message)>;

                           struct error_info_t
                           {
                              strong::ipc::id ipc;
                              common::transaction::ID trid;
                           };

                           auto error_info = error_info_t{ .ipc = message.process.ipc, .trid = message.trid};
                        
                           return state.tasks.create_unit( descriptor, message.correlation,
                           [ &state, ipc = message.process.ipc]( reply_type& message, strong::socket::id descriptor)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::transaction::resource::detail::create::task reply"};
                              log::debug( "message: ", message);

                              state.multiplex.send( ipc, message);
                           },
                           [ &state, error_info = std::move( error_info)]( casual::task::concurrent::message::task::Failed& message, strong::socket::id descriptor)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::transaction::resource::detail::create::task task::Failed"};

                              reply_type reply;
                              reply.correlation = message.correlation;
                              reply.trid = error_info.trid;
                              reply.state = code::xa::resource_fail;

                              state.multiplex.send( error_info.ipc, reply);
                           });
                        }
                        
                     } // detail::create

                     namespace basic
                     {
                        template< typename Message>
                        auto request( State& state)
                        {
                           return [&state]( Message& message, strong::ipc::descriptor::id descriptor)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::transaction::resource::basic::request"};
                              log::debug( "message: ", message);

                              auto tcp = state.connections.partner( descriptor);

                              tcp::send( state, tcp, message);
                              state.tasks.add( detail::create::task( state, message, tcp));
                           };
                        }  
                     } // basic

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
                  auto metric( State& state, const M& message, D&& destination, common::service::Code code)
                  {
                     Trace trace{ "gateway::group::outbound::handle::local::internal::service::call::metric"};

                     common::message::event::service::Metric metric;
                     { 
                        metric.process = state.connections.process_handle( destination.descriptor);
                        metric.correlation = message.correlation;
                        metric.execution = message.execution;
                        metric.span =  destination.span;
                        metric.service = std::move( destination.service);
                        metric.parent = std::move( destination.parent);
                        metric.type = decltype( metric.type)::concurrent;
                        metric.code = code;
                        
                        metric.trid = std::move( destination.trid);
                        metric.start = destination.start;
                        metric.end = platform::time::clock::type::now();
                     }
                     state.metric.add( std::move( metric));
                     state.metric.maybe_metric( state, &handle::metric::service, &handle::metric::queue);
                  }

                  namespace call
                  {
                     namespace detail::send::error
                     {
                        void reply( State& state, strong::ipc::descriptor::id descriptor, const common::message::service::call::callee::Request& message, code::xatmi code)
                        {
                           if( flag::contains( message.flags, decltype( message.flags)::no_reply))
                              return;

                           auto reply = common::message::reverse::type( message);
                           reply.code.result = code;
                           
                           struct Destination
                           {
                              strong::ipc::descriptor::id descriptor;
                              strong::execution::span::id span;
                              std::string service;
                              execution::context::Parent parent;
                              strong::ipc::id ipc;
                              common::transaction::ID trid;
                              common::chronology::time_point start;
                           };

                           state.multiplex.send( message.process.ipc, reply);

                           // NOTE: since we swapped the spans before: 
                           //   message.parent.span is our current actual span. 
                           //   message.span is our parent span from upstream. 

                           service::metric( state, reply, 
                              Destination{ 
                                 .descriptor = descriptor,
                                 .span = message.parent.span, 
                                 .service = message.service.name, 
                                 .parent = { .span = message.span, .service = message.parent.service}, 
                                 .ipc = message.process.ipc,
                                 .trid = message.trid,
                                 .start = platform::time::clock::type::now()},
                              { .result = code});

                        }
                        
                     } // detail::send::error   

                     namespace detail::create
                     {
                        auto task( State& state, const common::message::service::call::callee::Request& message, strong::socket::id descriptor)
                        {
                           struct Destination
                           {
                              strong::socket::id descriptor;
                              strong::execution::span::id span;
                              std::string service;
                              execution::context::Parent parent;
                              strong::ipc::id ipc;
                              common::transaction::ID trid;
                              common::chronology::time_point start;
                           };

                           // NOTE: since we swapped the spans before:
                           //   message.parent.span is our current actual span.
                           //   message.span is our parent span from upstream.

                           auto shared = std::make_shared< Destination>( Destination{ 
                              .descriptor = descriptor,
                              .span = message.parent.span, 
                              .service = message.service.name, 
                              .parent = { .span = message.span, .service = message.parent.service},
                              .ipc = message.process.ipc, 
                              .trid = message.trid, 
                              .start = platform::time::clock::type::now()});
                           
                           return state.tasks.create_unit( descriptor, message.correlation,
                           [ &state, shared]( common::message::service::call::Reply& message, strong::socket::id descriptor)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::detail::create::task call::Reply"};
                              log::debug( "message: ", message);

                              state.multiplex.send( shared->ipc, message);

                              // we unadvertise the service if we get no_entry
                              if( message.code.result == decltype( message.code.result)::no_entry)
                                 service::unadvertise( state, descriptor, { shared->service});

                              internal::service::metric( state, message, std::move( *shared), message.code);
                           },
                           [ &state, shared]( casual::task::concurrent::message::task::Failed& message, strong::socket::id descriptor)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::detail::create::task task::Failed"};

                              common::message::service::call::Reply reply;
                              reply.correlation = message.correlation;
                              reply.code.result = common::code::xatmi::service_error;

                              state.multiplex.send( shared->ipc, reply);

                              service::metric( state, reply, std::move( *shared), reply.code);
                           });
                        }
                        
                     } // detail::create

                     auto request( State& state)
                     {
                        return [ &state]( common::message::service::call::callee::Request& message, strong::ipc::descriptor::id descriptor)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::internal::service::call::request"};
                           log::debug( "message: ", message);

                           // message.span is the span for this "invocation", hence the parent span for the
                           // downstream call. We still need to keep the span for ACK later, so we just 
                           // swap these two spans.
                           std::swap( message.span, message.parent.span);

                           // now what we need to ack later is:
                           // span = message.parent.span
                           // parent.span = message.span

                           // Check if we've has been called with the same correlation id before, 
                           // hence we are in a loop between gateways.
                           if( state.tasks.contains( message.correlation))
                           {
                              log::error( code::casual::invalid_semantics, "a call with the same correlation id is in flight - ", message.correlation, " - action: reply with ", code::xatmi::system   );
                              detail::send::error::reply( state, descriptor, message, code::xatmi::system);
                              return;
                           }

                           auto connection = state.connections.find_external( descriptor);
                           CASUAL_ASSERT( connection);

                           // we only prepare a reply task and associate the transaction if the call is NOT no_reply
                           if( ! flag::contains( message.flags, decltype( message.flags)::no_reply))
                           {
                              state.tasks.add( detail::create::task( state, std::as_const( message), connection->descriptor()));
                              transaction::associate_and_involve( state, std::as_const( message), connection->descriptor());
                           }

                           if( message::protocol::compatible< common::message::service::call::callee::Request>( connection->protocol()))
                              tcp::send( state, connection->descriptor(), message);
                           else if( message::protocol::compatible< common::message::service::call::v1_4::callee::Request>( connection->protocol()))
                              tcp::send( state, connection->descriptor(), message::protocol::transform::to< common::message::service::call::v1_4::callee::Request>( std::move( message)));
                           else
                              tcp::send( state, connection->descriptor(), message::protocol::transform::to< common::message::service::call::v1_2::callee::Request>( std::move( message)));

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
                              strong::socket::id descriptor;
                              strong::execution::span::id span;
                              std::string service;
                              execution::context::Parent parent;
                              strong::ipc::id ipc;
                              common::chronology::time_point start;
                              common::transaction::ID trid;
                           };

                           // NOTE: since we swapped the spans before:
                           //   message.parent.span is our current actual span.
                           //   message.span is our parent span from upstream.

                           auto shared = std::make_shared< Shared>( Shared{ 
                              .descriptor = descriptor,
                              .span = message.parent.span, 
                              .service = message.service.name,
                              .parent = { .span = message.span, .service = message.parent.service},
                              .ipc = message.process.ipc, 
                              .start = platform::time::clock::type::now(), 
                              .trid = message.trid});


                           return state.tasks.create_unit( descriptor, message.correlation, 
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

                                 // if the send indicate a terminated conversation, we end the task.
                                 if( message.duplex == decltype( message.duplex)::terminated)
                                 {
                                    service::metric( state, message, std::move( *shared), message.code);
                                    return task::concurrent::unit::Dispatch::done;
                                 }

                                 return task::concurrent::unit::Dispatch::pending;
                              },
                              [ &state, shared]( common::message::conversation::Disconnect& message, strong::socket::id descriptor)
                              {
                                 Trace trace{ "gateway::group::outbound::handle::local::internal::conversation::task Disconnect"};

                                 // this will be invoked from internal side. Only the one who connect, can disconnect.
                                 // We'll be done with the task, and send metric to SM

                                 tcp::send( state, descriptor, message);

                                 service::metric( state, message, std::move( *shared), { .result = code::xatmi::ok});

                                 return task::concurrent::unit::Dispatch::done;
                              }
                           );
                        }

                     } // detail::create

                     auto request( State& state)
                     {
                        return [ &state]( common::message::conversation::connect::callee::Request& message, strong::ipc::descriptor::id descriptor)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::internal::conversation::connect::request"};
                           log::debug( "message: ", message);

                           auto connection = state.connections.find_external( descriptor);
                           CASUAL_ASSERT( connection);

                           // message.span is the span for this "invocation", hence the parent span for the
                           // downstream call. We still need to keep the span for ACK later, so we just 
                           // swap these two spans.
                           std::swap( message.span, message.parent.span);

                           // now what we need to ack later is:
                           // span = message.parent.span
                           // parent.span = message.span

                           state.tasks.add( detail::create::task( state, message, connection->descriptor()));
                           transaction::associate_and_involve( state, message, connection->descriptor());

                           if( message::protocol::compatible< common::message::conversation::connect::callee::Request>( connection->protocol()))
                              tcp::send( state, connection->descriptor(), message);
                           else if( message::protocol::compatible< common::message::conversation::connect::v1_5::callee::Request>( connection->protocol()))
                              tcp::send( state, connection->descriptor(), message::protocol::transform::to< common::message::conversation::connect::v1_5::callee::Request>( std::move( message)));
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
                        log::debug( "message: ", message);

                        // this should en the task
                        state.tasks( message);
                     };
                  }

                  auto send( State& state)
                  {
                     return [&state]( common::message::conversation::callee::Send& message, strong::ipc::descriptor::id descriptor)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::internal::conversation::send"};
                        log::debug( "message: ", message);

                        auto connection = state.connections.find_external( descriptor);
                        CASUAL_ASSERT( connection);

                        if( message::protocol::compatible< common::message::conversation::callee::Send>( connection->protocol()))
                           tcp::send( state, connection->descriptor(), message);
                        else
                           tcp::send( state, connection->descriptor(), message::protocol::transform::to< common::message::conversation::v1_5::callee::Send>( std::move( message)));

                     };
                  }

               } // conversation

               namespace domain
               {
                  namespace discovery
                  {
                     namespace detail
                     {
                        void advertise( State& state, const casual::domain::message::discovery::Reply& message, strong::socket::id descriptor)
                        {
                           auto handle = state.connections.process_handle( descriptor);
                           CASUAL_ASSERT( handle);

                           auto information = state.connections.information( descriptor);
                           CASUAL_ASSERT( information);

                           if( ! message.content.services.empty())
                           {
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
                                 casual::queue::ipc::message::advertise::Queue result;
                                 result.name = queue.name;
                                 result.retry.count = queue.retry.count;
                                 result.retry.delay = queue.retry.delay;
                                 result.enable.enqueue = queue.enable.enqueue;
                                 result.enable.dequeue = queue.enable.dequeue;
                                 return result;
                              });

                              state.multiplex.send( ipc::manager::optional::queue(), request);
                           }

                        }
                     } // detail

                     namespace detail::create
                     {
                        auto task( State& state, const casual::domain::message::discovery::Request& message, strong::socket::id descriptor)
                        {                        
                           return state.tasks.create_unit( descriptor, message.correlation,
                           [ &state, ipc = message.process.ipc]( casual::domain::message::discovery::Reply& message, strong::socket::id descriptor)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::domain::discovery::detail::create::task Reply"};
                              log::debug( "message: ", message);

                              // increase hops for all services.
                              for( auto& service : message.content.services)
                                 ++service.property.hops;

                              detail::advertise( state, message, descriptor);

                              state.multiplex.send( ipc, message);
                           },
                           [ &state, ipc = message.process.ipc]( casual::task::concurrent::message::task::Failed& message, strong::socket::id descriptor)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::domain::discovery::detail::create::task task::Failed"};

                              casual::domain::message::discovery::Reply reply;
                              reply.correlation = message.correlation;

                              state.multiplex.send( ipc, reply);
                           });
                        }
                        
                     } // detail::create

                     auto request( State& state)
                     {
                        return [&state]( casual::domain::message::discovery::Request& message, strong::ipc::descriptor::id descriptor)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::internal::domain::discover::request"};
                           log::debug( "message: ", message);

                           auto tcp = state.connections.partner( descriptor);

                           state.tasks.add( detail::create::task( state, message, tcp));

                           tcp::send( state, tcp, message);
                        };
                     }

                     namespace topology::direct
                     {
                        namespace detail::create
                        {
                           auto task( State& state, casual::domain::message::discovery::Request& message, strong::socket::id descriptor)
                           {
                              // note that we don't keep track of any reply destinations, since caller does not expect any.
                              return state.tasks.create_unit( descriptor, message.correlation,
                                 [ &state]( casual::domain::message::discovery::Reply& message, strong::socket::id descriptor)
                                 {
                                    Trace trace{ "gateway::group::outbound::handle::local::internal::domain::discover::topology::direct::detail::create::task Reply"};
                                    log::debug( "message: ", message);

                                    // increase hops for all services.
                                    for( auto& service : message.content.services)
                                       ++service.property.hops;

                                    discovery::detail::advertise( state, message, descriptor);
                                 });
                           }
                        } // detail::create

                        auto explore( State& state)
                        {
                           //! Sent from _discovery_ when:
                           //! * there are new connections (either from our process, and/or some other "outbound")
                           //! * AND/OR there are implicit topology updates (either from our process, and/or some other "outbound")
                           //! * after some specific time, _discovery_ gathers "known", and send topology::direct::Explore to us (and other "outbounds")
                           return [ &state]( casual::domain::message::discovery::topology::direct::Explore& message, strong::ipc::descriptor::id descriptor)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::domain::discover::topology::direct::explore"};
                              log::debug( "message: ", message);

                              auto tcp = state.connections.partner( descriptor);

                              if( state.runlevel > decltype( state.runlevel())::running)
                                 return;

                              if( state.pending.dissociating.contains( tcp))
                                 return;

                              casual::domain::message::discovery::Request request;
                              request.correlation = message.correlation;
                              request.content = std::move( message.content);
                              request.domain = common::domain::identity();
                              
                              tcp::send( state, tcp, request);
                              
                              state.tasks.add( detail::create::task( state, request, tcp));
                           };

                        }
                     } // topology::direct

                  } // discovery

               } // domain

               namespace queue
               {
                  namespace detail
                  {
                     struct Shared
                     {
                        strong::ipc::id destination;
                        strong::correlation::id correlation;
                        std::string name;
                        casual::queue::ipc::message::group::metric::remote::Direction direction;
                        chronology::time_point start;
                     };
                  
                     void unadvertise( State& state, strong::socket::id descriptor, std::string queue)
                     {
                        if( auto handle = state.connections.process_handle( descriptor))
                        {
                           casual::queue::ipc::message::Advertise unadvertise{ handle};
                           unadvertise.alias = instance::alias();
                           unadvertise.queues.remove.push_back( std::move( queue));
                           state.multiplex.send( ipc::manager::optional::queue(), unadvertise);
                        }
                        else
                           log::error( code::casual::invalid_semantics, "failed to unadvertise - could not find ipc partner to ", descriptor);
                     }

                     void metric( State& state, const Shared& shared, strong::socket::id descriptor, auto code)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::internal::queue::detail::metric"};

                        auto entry = casual::queue::ipc::message::group::metric::remote::Entry{
                           .process = state.connections.process_handle( descriptor),
                           .correlation = shared.correlation,
                           .queue = shared.name,
                           .start = shared.start,
                           .end = platform::time::clock::type::now(),
                           .code = code,
                           .direction = shared.direction
                        };

                        log::debug( "entry: ", entry);

                        state.metric.add( std::move( entry));
                        state.metric.maybe_metric( state, &handle::metric::service, &handle::metric::queue);
                     }

                     namespace create
                     {
                        auto task( State& state, auto& message, strong::socket::id descriptor)
                        {
                           using reply_type = common::message::reverse::type_t< decltype( message)>;

                           constexpr auto direction = []()
                           {
                              if constexpr( std::is_same_v< decltype( message), casual::queue::ipc::message::group::enqueue::Request>)
                                 return casual::queue::ipc::message::group::metric::remote::Direction::enqueue;
                              else
                                 return casual::queue::ipc::message::group::metric::remote::Direction::dequeue;
                           }();

                           auto shared = std::make_shared< Shared>( Shared{ 
                              .destination = message.process.ipc,
                              .correlation = message.correlation,
                              .name = message.name, 
                              .direction = direction,
                              .start = platform::time::clock::type::now(),
                           });
                        
                           return state.tasks.create_unit( descriptor, message.correlation,
                           [ &state, shared]( reply_type& message, strong::socket::id descriptor)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::queue::detail::create::task Reply"};
                              log::debug( "message: ", message);

                              // if the reply has code::queue::no_queue, we need to unadvertise the queue.
                              if( message.code == code::queue::no_queue)
                                 detail::unadvertise( state, descriptor, shared->name);

                              state.multiplex.send( shared->destination, message);

                              detail::metric( state, *shared, descriptor, message.code);

                           },
                           [ &state, shared]( casual::task::concurrent::message::task::Failed& message, strong::socket::id descriptor)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::queue::detail::create::task task::Failed"};

                              reply_type reply;
                              reply.correlation = message.correlation;
                              reply.code = code::queue::system;

                              detail::unadvertise( state, descriptor, shared->name);

                              state.multiplex.send( shared->destination, reply);

                              detail::metric( state, *shared, descriptor, code::queue::system);
                           });
                        }
                        
                     } // create
  
                     auto send( State& state, auto&& message, strong::socket::id tcp)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::internal::queue::detail::send"};
                        log::debug( "message: ", message);

                        tcp::send( state, tcp, message);
                        state.tasks.add( detail::create::task( state, message, tcp));

                        transaction::associate_and_involve( state, message, tcp);
                     }
                     
                  } // detail

                  namespace enqueue
                  {
                     auto request( State& state)
                     {
                        return [ &state]( casual::queue::ipc::message::group::enqueue::Request& message, strong::ipc::descriptor::id descriptor)
                        {
                           auto connection = state.connections.find_external( descriptor);
                           CASUAL_ASSERT( connection);

                           if( message::protocol::compatible< casual::queue::ipc::message::group::enqueue::Request>( connection->protocol()))
                           {
                              detail::send( state, message, connection->descriptor());
                           }
                           else if( message::protocol::compatible< casual::queue::ipc::message::group::enqueue::v1_5::Request>( connection->protocol()))
                           {
                              // this works because v1_5 has reverse type to the regular Reply.
                              detail::send( state, 
                                 message::protocol::transform::to< casual::queue::ipc::message::group::enqueue::v1_5::Request>( std::move( message)), 
                                 connection->descriptor());
                           }
                        };
                     }

                  } // enqueue

                  namespace dequeue
                  {                                          
                     auto request( State& state)
                     {
                        return [&state]( casual::queue::ipc::message::group::dequeue::Request& message, strong::ipc::descriptor::id descriptor)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::internal::queue::dequeue::request"};
                           log::debug( "message: ", message);

                           detail::send( state, message, state.connections.partner( descriptor));
                        };
                     } 

                  } // dequeue
               } // queue

               namespace connection::dissociate
               {
                  template< typename Message>
                  auto basic_task( State& state)
                  {
                     return [ &state]( Message& message)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::internal::connection::dissociate::basic_task"};
                        common::log::debug( "message: ", message);

                        state.pending.dissociating( message);
                        common::log::debug( "state.pending.dissociating: ", state.pending.dissociating);
                     };
                  }

                  namespace service
                  {
                     auto reply = basic_task< common::message::service::concurrent::instance::disassociate::Reply>;   
                  } // service

                  namespace queue
                  {
                     auto reply = basic_task< casual::queue::ipc::message::external::disassociate::Reply>;
                  } // queue

                  namespace transaction
                  {
                     auto reply = basic_task< common::message::transaction::resource::external::disassociate::Reply>;
                  } // transaction
                  
               } // connection::dissociate

               
            } // internal

            namespace disassociate
            {
               namespace detail::create
               {
                  struct Shared
                  {
                     state::disconnect::Directive directive;
                     bool sm_done = false;
                     bool qm_done = false;
                     common::strong::correlation::id correlation;
                     // possible reply to send back to external inbound
                     std::optional< gateway::message::domain::disconnect::Reply> reply;

                     bool done() const { return sm_done && qm_done;}
                  };

                  auto remove_connection( State& state, const Shared& shared, common::strong::socket::id descriptor)
                  {
                     Trace trace{ "gateway::group::outbound::handle::local::internal::disassociate::detail::create::remove_connection_if_done"};

                     // is an inbound waiting for a disconnect reply?
                     if( shared.reply)
                        tcp::send( state, descriptor, *shared.reply);

                     // we need to send an ipc-destroyed event, so other can disassociate stuff with the ipc
                     // NOTE: the only thing that needs this right now is TM to get rid of state about our resource.
                     if( auto handle = state.connections.process_handle( descriptor))
                        common::event::send( common::message::event::ipc::Destroyed{ handle});

                     {
                        auto information = state.connections.information( descriptor);
                        casual::assertion( information, "failed to find information for descriptor: ", descriptor);

                        log::information( "connection to '", information->domain.name, "' closed - address: ", information->address);
                     }

                     // its safe to remove the connection now, since all managers have disassociated the resource.
                     auto reconnect = state.extract( descriptor);

                     // NOTE: this should not be needed, since the connection should be "clean".
                     state.tasks.failed( descriptor);

                     //! if the pending directive is reconnect, we push the reconnect message to our device
                     //! and let a specialized handler handle the reconnect. It's different between 
                     //! reverse outbound and regular outbound.
                     if( shared.directive == state::disconnect::Directive::reconnect)
                        ipc::inbound().push( std::move( reconnect));

                     return task::concurrent::unit::Dispatch::done;
                  };

                  auto send_tm( State& state, const Shared& shared, common::strong::socket::id descriptor)
                  {
                     if( shared.done())
                     {   
                        auto handle = state.connections.process_handle( descriptor);
                        casual::assertion( handle, "failed to find handle for ", descriptor);

                        common::message::transaction::resource::external::disassociate::Request request{ handle};
                        request.correlation = shared.correlation;
                        state.multiplex.send( ipc::manager::transaction(), request);
                     }

                     // always pending, only TM reply will be done
                     return task::concurrent::unit::Dispatch::pending;
                  }

                  auto task( State& state, common::strong::socket::id descriptor, state::disconnect::Directive directive, std::optional< gateway::message::domain::disconnect::Reply> reply)
                  {
                     Trace trace{ "gateway::group::outbound::handle::local::internal::disassociate::detail::create::task"};

                     const auto correlation = strong::correlation::id::generate();
                     const auto handle = state.connections.process_handle( descriptor);
                     casual::assertion( handle, "failed to find handle for ", descriptor);

                     auto shared = std::make_shared< Shared>( Shared{ .directive = directive});
                     shared->correlation = correlation;
                     shared->reply = std::move( reply);

                     // make sure to unregister the connection from discovery, to avoid any new discovery stuff to be sent to this connection.
                     {
                        auto device = state.connections.find_internal( descriptor);
                        casual::assertion( device, "failed to find internal connection for descriptor: ", descriptor);

                        // we register with empty abilities -> unregister
                        casual::domain::discovery::provider::registration( *device, casual::domain::discovery::provider::Ability::absent);
                     }
                     
                     // SM
                     {
                        common::message::service::concurrent::instance::disassociate::Request request{ handle};
                        request.correlation = correlation;
                        state.multiplex.send( ipc::manager::service(), request);
                     }
                     // QM
                     {
                        casual::queue::ipc::message::external::disassociate::Request request{ handle};
                        request.correlation = correlation;
                        
                        if( ! state.multiplex.send( ipc::manager::optional::queue(), request))
                        {
                           log::debug( "queue-manager is not on-line");
                           shared->qm_done = true;
                        }
                     }
                     // TM
                     // we send the disassociate to TM when we've got reply from SM and potentially QM
                     // to eliminate the case where SM has pending calls, but the calls hav not arrived to us
                     // yet. TM could then just send a reply that our resource is disassociated, but will be
                     // when the calls arrive (same for QM).


                     return state.tasks.create_unit( descriptor, correlation, 
                     [ &state, shared]( const common::message::service::concurrent::instance::disassociate::Reply& message, strong::socket::id descriptor)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::internal::disassociate::detail::create::task service reply"};

                        shared->sm_done = true;
                        return send_tm( state, *shared, descriptor);
                     },
                     [ &state, shared]( const casual::queue::ipc::message::external::disassociate::Reply& message, strong::socket::id descriptor)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::internal::disassociate::detail::create::task queue reply"};

                        shared->qm_done = true;
                        return send_tm( state, *shared, descriptor);
                     },
                     [ &state, shared]( const common::message::transaction::resource::external::disassociate::Reply& message, strong::socket::id descriptor)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::internal::disassociate::detail::create::task transaction reply"};

                        // we're done, remove the connection.
                        return remove_connection( state, *shared, descriptor);
                     });
                  }
                  
               } // detail::create

               void connection( State& state, common::strong::socket::id descriptor, state::disconnect::Directive directive, std::optional< gateway::message::domain::disconnect::Reply> reply = {})
               {
                  Trace trace{ "gateway::group::outbound::handle::local::internal::disassociate::connection"};
                  log::debug( "descriptor: ", descriptor);

                  // We could already have a pending disconnect for this descriptor, in that case we don't cancel it again.
                  // however, we need to reply if present (this should not happen).
                  if( state.pending.dissociating.contains( descriptor))
                  {
                     log::debug( "connection already pending dissociate ", descriptor);

                     if( reply)
                        tcp::send( state, descriptor, *reply);
                        
                     return;
                  }
                     
                  state.pending.dissociating.add( detail::create::task( state, descriptor, directive, std::move( reply)));
               }
               
            } // disassociate

            namespace external
            {
               template< typename Message>
               auto basic_task( State& state)
               {
                  return [ &state]( Message& message)
                  {
                     Trace trace{ "gateway::group::outbound::handle::local::external::basic_task"};
                     common::log::debug( "message: ", message);

                     state.tasks( message);
                     common::log::debug( "state.tasks: ", state.tasks);
                  };
               }
               

               namespace disconnect
               {
                  auto request( State& state)
                  {
                     return [&state]( const gateway::message::domain::disconnect::Request& message, strong::socket::id descriptor)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::external::disconnect::request"};
                        log::debug( "message: ", message);

                        disassociate::connection( state, descriptor, state::disconnect::Directive::reconnect, common::message::reverse::type( message));
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
                              log::debug( "message: ", message);

                              state.tasks( message::protocol::transform::from( std::move( message)));
                           };
                        }
                     } // v1_2

                     namespace v1_4
                     {
                        auto reply( State& state)
                        {
                           return [ &state]( common::message::service::call::v1_4::Reply message)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::external::service::call::v1_4::reply"};
                              log::debug( "message: ", message);

                              state.tasks( message::protocol::transform::from( std::move( message)));
                           };
                        }
                        
                     } // v1_4
                  } // call

               } // service

               namespace conversation
               {
                  namespace connect
                  {
                     auto reply = basic_task< common::message::conversation::connect::Reply>;

                  } // connect

                  auto send = basic_task< common::message::conversation::callee::Send>;

                  namespace v1_5
                  {
                     auto send( State& state)
                     {
                        return [ &state]( common::message::conversation::v1_5::callee::Send message)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::external::conversation::callee::v1_5::send"};
                           log::debug( "message: ", message);

                           state.tasks( message::protocol::transform::from( std::move( message)));
                        };
                     }

                     
                  } // v1_5

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
                              log::debug( "message: ", message);

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
                              log::debug( "message: ", message);

                              state.tasks( message::protocol::transform::from( std::move( message)));
                           };
                        }   
                     } // v1_2

                     namespace v1_5
                     {
                        auto reply( State& state)
                        {
                           return [ &state]( casual::queue::ipc::message::group::dequeue::v1_5::Reply& message, strong::socket::id descriptor)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::external::queue::dequeue::v1_5::reply"};
                              log::debug( "message: ", message);

                              state.tasks( message::protocol::transform::from( std::move( message)));
                           };
                        }
                     } // v1_5

                  } // dequeue
               } // queue

               namespace transaction::resource
               {
                  namespace prepare
                  {
                     auto reply = external::basic_task< common::message::transaction::resource::prepare::Reply>;

                  } // prepare

                  namespace commit
                  {
                     auto reply = external::basic_task< common::message::transaction::resource::commit::Reply>;

                  } // commit

                  namespace rollback
                  {
                     auto reply = external::basic_task< common::message::transaction::resource::rollback::Reply>;
                     
                  } // rollback

               } // transaction::resource

               namespace domain::discovery
               {
                  auto reply = basic_task< casual::domain::message::discovery::Reply>;

                  namespace v1_3
                  {
                     auto reply( State& state)
                     {
                        return [ &state]( casual::domain::message::discovery::v1_3::Reply&& message, strong::socket::id descriptor)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::external::domain::discovery::v1_3::reply"};
                           log::debug( "message: ", message);

                           state.tasks( message::protocol::transform::from( std::move( message)));
                        };
                     }
                  } // v1_3

                  namespace topology
                  {
                     //! we get this from inbounds that are configured with _discovery forward_ and the domain topology has 
                     //! been updated.
                     auto update( State& state)
                     {
                        return [ &state]( casual::domain::message::discovery::topology::implicit::Update&& message, strong::socket::id descriptor)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::external::domain::discover::topology::update"};
                           log::debug( "message: ", message);

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
               namespace domain
               {
                  auto connected( State& state)
                  {
                     return [ &state]( const gateway::message::domain::Connected& message)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::internal::domain::connected"};
                        common::log::debug( "message: ", message);

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

               namespace event::transaction
               {
                  auto disassociate( State& state)
                  {
                     return [ &state]( const common::message::event::transaction::Disassociate& message)
                     {
                        Trace trace{ "gateway::outbound::handle:::event::transaction::disassociate"};
                        log::debug( "message: ", message);

                        state.pending.transactions.remove( message.gtrid.range());
                     };
                  }
               } // event::transaction

               namespace connection
               {
                  auto lost( State& state)
                  {
                     return [ &state]( const gateway::message::connection::Lost& message)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::internal::connection::lost"};
                        log::debug( "message: ", message);

                        auto information = state.connections.information( message.descriptor);
                        casual::assertion( information, "failed to find information for descriptor: ", message.descriptor);

                        // fail potential pending tasks (calls and such).
                        state.tasks.failed( message.descriptor);

                        if( ! state.pending.dissociating.contains( message.descriptor))
                        {
                           log::information( message.code, " lost connection to: '", information->domain.name, "' - address: ", information->address);

                           // we have lost the connection, but we have not disassociated it yet.
                           // we only need to do this once. But during the first disassociate, we still could
                           // get ipc messages that will trigger this lost event again.
                           local::disassociate::connection( state, message.descriptor, state::disconnect::Directive::reconnect);
                        }
                        
                     };
                  }
               } // connection
               
            } // management

         } // <unnamed>
      } // local

      management_handler management( State& state)
      {
         return management_handler{
            common::event::listener( 
               handle::local::management::event::transaction::disassociate( state)
            ),
            local::management::domain::connected( state),
            local::management::connection::lost( state)
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

            // connection
            local::internal::connection::dissociate::service::reply( state),
            local::internal::connection::dissociate::queue::reply( state),
            local::internal::connection::dissociate::transaction::reply( state)
         };
      }

      external_handler external( State& state)
      {
         return external_handler{
            local::external::disconnect::request( state),
            
            // service
            local::external::service::call::reply( state),
            local::external::service::call::v1_2::reply( state),
            local::external::service::call::v1_4::reply( state),

            // conversation
            local::external::conversation::connect::reply( state),
            local::external::conversation::send( state),
            local::external::conversation::v1_5::send( state),

            // queue
            local::external::queue::enqueue::reply( state),
            local::external::queue::enqueue::v1_2::reply( state),
            local::external::queue::dequeue::reply( state),
            local::external::queue::dequeue::v1_5::reply( state),
            local::external::queue::dequeue::v1_2::reply( state),

            // transaction
            local::external::transaction::resource::prepare::reply( state),
            local::external::transaction::resource::commit::reply( state),
            local::external::transaction::resource::rollback::reply( state),

            // discover
            local::external::domain::discovery::reply( state),
            local::external::domain::discovery::v1_3::reply( state),
            local::external::domain::discovery::topology::update( state)
         };
      }

     
      namespace connection
      {
         
         void disconnect( State& state, common::strong::socket::id descriptor)
         {
            Trace trace{ "gateway::group::outbound::handle::connection::disconnect"};
            log::debug( "descriptor: ", descriptor);

            local::disassociate::connection( state, descriptor, state::disconnect::Directive::reconnect);
         }

         void remove( State& state, common::strong::socket::id descriptor)
         {
            Trace trace{ "gateway::group::outbound::handle::connection::remove"};
            log::debug( "descriptor: ", descriptor);

            local::disassociate::connection( state, descriptor, state::disconnect::Directive::remove);
         }

         
      } // connection

      namespace advertise
      {
         void connections( State& state)
         {
            Trace trace{ "gateway::group::outbound::handle::advertise::connections"};

            auto advertise = [ &state]( auto& information)
            {
               auto handle = state.connections.process_handle( information.descriptor);
               {
                  common::message::service::concurrent::Advertise request{ handle};
                  // only update instance information
                  request.directive = decltype( request.directive)::instance;
                  request.alias = state.alias;
                  request.description = information.domain.name;
                  request.order = state.order;
                  state.multiplex.send( ipc::manager::service(), request);
               }
               {
                  casual::queue::ipc::message::Advertise request{ handle};
                  // only update instance information
                  request.directive = decltype( request.directive)::instance;
                  request.alias = state.alias;
                  request.description = information.domain.name;
                  request.order = state.order;
                  state.multiplex.send( ipc::manager::optional::queue(), request);
               }
            };

            algorithm::for_each( state.connections.information(), advertise);
         }
         
      } // advertise

      void idle( State& state)
      {
         Trace trace{ "gateway::group::outbound::handle::idle"};

         // we need to check/send metric, we don't know when we're about to be called again.
         state.metric.force_metric( state, &handle::metric::service, &handle::metric::queue);
      }

      void shutdown( State& state)
      {
         Trace trace{ "gateway::group::outbound::handle::shutdown"};

         state.runlevel = state::Runlevel::shutdown;

         for( auto descriptor : state.connections.external_descriptors())
            handle::connection::remove( state, descriptor);

         // send metric for good measure
         state.metric.force_metric( state, &handle::metric::service, &handle::metric::queue);

         log::debug( "state: ", state);
      }

      void abort( State& state)
      {
         Trace trace{ "gateway::group::outbound::handle::abort"};
         log::error( code::casual::abort, "best effort shutdown");

         state.runlevel = state::Runlevel::error;

         for( auto descriptor : state.connections.external_descriptors())
            state.tasks.failed( descriptor);

         state.connections.clear( state.directive);
      }

      namespace metric
      {
         void service( State& state, const common::message::event::service::Calls& metric)
         {
            Trace trace{ "gateway::group::outbound::handle::metric::service"};
            log::debug( "metric: ", metric);

            state.multiplex.send( ipc::manager::service(), metric);
         }

         void queue( State& state, const queue::ipc::message::group::metric::remote::Entries& metric)
         {
            Trace trace{ "gateway::group::outbound::handle::metric::queue"};
            log::debug( "metric: ", metric);

            state.multiplex.send( ipc::manager::optional::queue(), metric);
         }

      } // metric

   } // gateway::group::outbound::handle
} // casual
