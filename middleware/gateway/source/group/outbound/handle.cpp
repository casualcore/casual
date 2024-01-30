//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/outbound/handle.h"
#include "gateway/group/tcp.h"
#include "gateway/group/ipc.h"

#include "gateway/message.h"
#include "gateway/common.h"

#include "domain/discovery/api.h"

#include "common/communication/instance.h"
#include "common/message/dispatch/handle.h"
#include "common/message/internal.h"
#include "common/event/send.h"
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

               namespace transaction
               {
                  template< typename M> 
                  void involved( M&& message)
                  {
                     if( message.trid)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::internal::transaction::involved"};
                        log::line( verbose::log, "message: ", message);

                        // We can't really get rid of this (now), We need to make sure TM get's the involve message
                        // before we do anything else. Otherwise the transaction might get committed and TM does not know
                        // about this _involved external resource_.
                        ipc::flush::send( 
                           ipc::manager::transaction(),
                           common::message::transaction::resource::external::involved::create( message));
                     }
                  }

                  namespace resource
                  {  
                     namespace request
                     {               
                        template< typename Message>
                           requires concepts::any_of< Message, // sanity for the constexpr if.
                              common::message::transaction::resource::prepare::Request,
                              common::message::transaction::resource::commit::Request,
                              common::message::transaction::resource::rollback::Request>
                        void fan_out( State& state, auto& fan_out, Message& message) 
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::internal::transaction::resource::fan_out"};
                           log::line( verbose::log, "message: ", message);

                           // make sure we get unique correlations for the coordination
                           auto correlation = std::exchange( message.correlation, {});

                           // keep track of failed send.
                           std::vector< strong::file::descriptor::id> failed_connections;

                           auto send_requests = [ &state, &failed_connections, &fan_out]( auto& message)
                           {
                              auto result = fan_out.empty_pendings();
                              auto connections = state.lookup.connections( common::transaction::id::range::global( message.trid));

                              log::line( verbose::log, "connections: ", connections);

                              auto send_request = [ &state, &result, &message, &failed_connections]( auto descriptor)
                              {
                                 auto failed = [ &result, &failed_connections]( auto descriptor)
                                 {
                                    result.emplace_back( strong::correlation::id{}, descriptor);
                                    failed_connections.push_back( descriptor);
                                 };
                                 
                                 // if not a valid descriptor, the associated connection has failed (connection lost)
                                 if( ! descriptor)
                                    failed( descriptor);
                                 else if( auto correlation = local::tcp::send( state, descriptor, message))
                                    result.emplace_back( correlation, descriptor);
                                 else
                                    failed( descriptor);
                              };

                              algorithm::for_each( connections, send_request);

                              return result;
                           };

                           auto callback = [ &state, correlation, trid = message.trid, ipc = message.process.ipc, rm = message.resource]( auto&& replies, auto&& outcome)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::transaction::resource::fan_out callback"};
                              log::line( verbose::log, "replies: ", replies, ", outcome: ", outcome);

                              auto most_severe = []( auto result, auto& reply)
                              {
                                 return code::severest( result, reply.state);
                              };

                              common::message::reverse::type_t< Message> reply;
                              reply.correlation = correlation;
                              reply.trid = trid;
                              reply.resource = rm;

                              // normalize/accumulate the state from several replies to one

                              if( algorithm::find( outcome, decltype( range::front( outcome).state)::failed))
                                 reply.state = code::xa::resource_fail;
                              else
                                 reply.state = algorithm::accumulate( replies, code::xa::read_only, most_severe);

                              // clean up state. It could be failed connections that is not removed by replies (of course)
                              if constexpr( std::same_as< Message, common::message::transaction::resource::prepare::Request>)
                                 if( reply.state == code::xa::read_only)
                                    state.lookup.remove( common::transaction::id::range::global( trid));
                              if constexpr( concepts::any_of< Message, common::message::transaction::resource::commit::Request>)
                                 state.lookup.remove( common::transaction::id::range::global( trid));
                              if constexpr( std::same_as< Message, common::message::transaction::resource::rollback::Request>)
                                 state.lookup.remove( common::transaction::id::range::global( trid));

                              log::line( verbose::log, "reply: ", reply);
                              state.multiplex.send( ipc, reply);
                           };

                           // add the pending via call and the callback.
                           fan_out( send_requests( message), std::move( callback));

                           // take care of possible failed send
                           for( auto connection : failed_connections)
                              fan_out.failed( connection);

                        }
                     } // request

                     namespace basic
                     {
                        template< typename Message>
                        auto request( State& state)
                        {
                           return [ &state]( Message& message)
                           {
                              if constexpr( std::same_as< Message, common::message::transaction::resource::prepare::Request>)
                                 resource::request::fan_out( state, state.coordinate.transaction.prepare, message);
                              if constexpr( std::same_as< Message, common::message::transaction::resource::commit::Request>)
                                 resource::request::fan_out( state, state.coordinate.transaction.commit, message);
                              if constexpr( std::same_as< Message, common::message::transaction::resource::rollback::Request>)
                                 resource::request::fan_out( state, state.coordinate.transaction.rollback, message);
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
                  namespace call
                  {
                     template< typename D>
                     auto metric( State& state, const D& destination, 
                        state::service::Pending::Call call, const common::transaction::ID& trid, code::xatmi code)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::internal::service::call::metric"};

                        common::message::event::service::Metric metric;
                        {   
                           metric.process = common::process::handle();
                           metric.correlation = call.correlation;
                           metric.execution = destination.execution;
                           metric.service = std::move( call.service);
                           metric.parent = std::move( call.parent);
                           metric.type = decltype( metric.type)::concurrent;
                           metric.code = code;
                           
                           metric.trid = trid;
                           metric.start = call.start;
                           metric.end = platform::time::clock::type::now();
                        }
                        state.service.add( std::move( metric));

                        // possible send metric
                        state.service.maybe_metric( state, []( auto& state, auto& message)
                        {
                           state.multiplex.send( ipc::manager::service(), message);
                        });
                     }

                     namespace detail::send::error
                     {
                        template< typename V>
                        auto ipc( const V& value) -> decltype( ( value.process.ipc)) { return value.process.ipc;}
                        template< typename V>
                        auto ipc( const V& value) -> decltype( ( value.ipc)) { return value.ipc;}

                        template< typename M, typename P>
                        void reply( State& state, M&& destination, P pending, const common::transaction::ID& trid, code::xatmi code)
                        {
                           common::message::service::call::Reply reply;
                           reply.correlation = destination.correlation;
                           reply.execution = destination.execution;
                           
                           if( trid)
                           {
                              reply.transaction.trid = trid;
                              reply.transaction.state = decltype( reply.transaction.state)::error;
                           }

                           reply.code.result = code;

                           log::line( verbose::log, "reply: ", reply);
                           state.multiplex.send( detail::send::error::ipc( destination), reply);

                           service::call::metric( state, destination, std::move( pending), trid, code);
                        };
                        
                     } // detail::send::error                

                     auto request( State& state)
                     {
                        return [&state]( common::message::service::call::callee::Request& message)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::internal::service::call::request"};
                           log::line( verbose::log, "message: ", message);

                           //! TODO maintainence - this need to be simplified

                           auto pending = state::service::Pending::Call{ message};

                           auto lookup = state.lookup.service( message.service.name, message.trid);
                           log::line( verbose::log, "lookup: ", lookup);

                           // check if we've has been called with the same correlation id before, hence we are in a
                           // loop between gateways.
                           if( state.route.contains( message.correlation))
                           {
                              log::line( log::category::error, code::casual::invalid_semantics, " a call with the same correlation id is in flight - ", message.correlation, " - action: reply with ", code::xatmi::system   );
                              detail::send::error::reply( state, message, std::move( pending), message.trid, code::xatmi::system);
                              return;
                           }

                           if( ! lookup.connection)
                           {
                              log::line( log::category::error, code::casual::invalid_semantics, " failed to look up service '", message.service.name, "' - action: reply with ", code::xatmi::no_entry);
                              detail::send::error::reply( state, message, std::move( pending), message.trid, code::xatmi::no_entry);
                              return;
                           }


                           auto create_error_callback = []( auto& state, auto& message) -> state::route::error::callback::type
                           {
                              if( message.flags.exist( common::message::service::call::request::Flag::no_reply))
                                 return {};

                              return [ &state, trid = message.trid]( auto& destination)
                              {
                                 auto pending = state.service.consume( destination.correlation);
                                 detail::send::error::reply( state, destination, std::move( pending), trid, code::xatmi::system);
                              };
                           };

                           auto route = state::route::Point{ message, lookup.connection, create_error_callback( state, message)};

                           // notify TM that we act as a "resource" and are involved in the transaction
                           if( lookup.new_transaction)
                              transaction::involved( message);
                              
                           if( ! message.flags.exist( common::message::service::call::request::Flag::no_reply))
                           {
                              state.service.add( std::move( pending));
                              state.route.add( std::move( route));
                           }

                           tcp::send( state, lookup.connection, message);
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
                           Trace trace{ "gateway::group::outbound::handle::local::internal::conversation::connect::request"};
                           log::line( verbose::log, "message: ", message);

                           auto lookup = state.lookup.service( message.service.name, message.trid);

                           // notify TM that this "resource" is involved in the branched transaction
                           if( lookup.new_transaction )
                              transaction::involved( message);

                           state.route.add( message, lookup.connection, [ &state]( auto& destination)
                           {
                              common::message::conversation::connect::Reply reply;
                              reply.correlation = destination.correlation;
                              reply.execution = destination.execution;
                              reply.code.result = decltype( reply.code.result)::system;
                              state.multiplex.send( destination.ipc, reply);
                           });

                           tcp::send( state, lookup.connection, message);
                        };
                     }

                  } // connect


                  auto disconnect( State& state)
                  {
                     return [&state]( common::message::conversation::Disconnect& message)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::internal::conversation::disconnect"};
                        log::line( verbose::log, "message: ", message);
                        
                        if( auto point = state.route.consume( message.correlation))
                        {
                           tcp::send( state, point.connection, message);
                        }
                        else
                        {
                           log::line( log::category::error, code::casual::internal_correlation, " failed to correlate internal::conversation::send [", message.correlation, "] - action: ignore");
                           log::line( verbose::log, "state.route: ", state.route);
                        }

                     };
                  }

                  auto send( State& state)
                  {
                     return [&state]( common::message::conversation::callee::Send& message)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::internal::conversation::send"};
                        log::line( verbose::log, "message: ", message);

                        if( auto found = algorithm::find( state.route.points(), message.correlation))
                        {
                           tcp::send( state, found->connection, message);
                        }
                        else
                        {
                           log::line( log::category::error, code::casual::internal_correlation, " failed to correlate internal::conversation::send [", message.correlation, "] - action: ignore");
                           log::line( verbose::log, "state.route: ", state.route);
                        }
                     };
                  }

               } // conversation

               namespace domain
               {
                  namespace discovery
                  {
                     namespace detail
                     {
                        namespace advertise
                        {
                           template< typename R, typename O>
                           auto replies( State& state, R& replies, O&& outcome)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::domain::discover::detail::advertise::replies"};

                              auto advertise_connection = [&state]( auto& reply, auto connection)
                              {

                                 auto advertise = state.lookup.add( 
                                    connection, 
                                    algorithm::transform( reply.content.services, []( auto& service){ return state::lookup::Resource{ service.name, service.property.hops};}), 
                                    algorithm::transform( reply.content.queues, []( auto& queue){ return state::lookup::Resource{ queue.name, 1};}));


                                 auto equal_name = []( auto& lhs, auto& rhs){ return lhs.name == rhs;};

                                 // we need to intersect whit the actual 'resource' information with what the state says we should advertise.
                                 // (we do not know the _information_, and don't really care)

                                 if( auto services = std::get< 0>( algorithm::intersection( reply.content.services, advertise.services, equal_name)))
                                 {
                                    common::message::service::concurrent::Advertise request{ common::process::handle()};
                                    request.alias = instance::alias();
                                    request.order = state.order;
                                    algorithm::copy( services, request.services.add);

                                    state.multiplex.send( ipc::manager::service(), request);
                                 }

                                 if( auto queues = std::get< 0>( algorithm::intersection( reply.content.queues, advertise.queues, equal_name)))
                                 {
                                    casual::queue::ipc::message::Advertise request{ common::process::handle()};
                                    request.order = state.order;
                                    request.queues.add = algorithm::transform( queues, []( auto& queue)
                                    {
                                       return casual::queue::ipc::message::advertise::Queue{ queue.name, queue.retries};
                                    });

                                    state.multiplex.send( ipc::manager::optional::queue(), request);
                                 }
                              };

                              auto succeeded = algorithm::filter( outcome, []( auto& value){ return value.state == decltype( value.state)::received;});

                              for( auto& reply : replies)
                                 if( auto found = algorithm::find( succeeded, reply.correlation))
                                    advertise_connection( reply, found->id);

                           };
                        } // advertise
                        
                     } // detail

                     auto request( State& state)
                     {
                        return [&state]( casual::domain::message::discovery::Request& message)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::internal::domain::discover::request"};
                           log::line( verbose::log, "message: ", message);

                           if( state.runlevel > decltype( state.runlevel())::running)
                           {
                              log::line( log, "outbound is in shutdown mode - action: reply with empty discovery");

                              state.multiplex.send( message.process.ipc, common::message::reverse::type( message));
                              return;
                           }

                           // make sure we get unique correlations for the coordination
                           auto correlation = std::exchange( message.correlation, {});

                           auto send_requests = []( State& state, auto& message)
                           {
                              auto result = state.coordinate.discovery.empty_pendings();

                              auto send_request = [ &state, &result, &message]( auto descriptor)
                              {
                                 if( algorithm::find( state.disconnecting, descriptor))
                                    return;

                                 // check if the connection is the same domain that instigated the discovery
                                 if( auto found = state.external.information( descriptor))
                                    if( found->domain.id == message.domain.id)
                                       return;

                                 if( auto correlation = local::tcp::send( state, descriptor, message))
                                    result.emplace_back( correlation, descriptor);
                              };

                              algorithm::for_each( state.external.external_descriptors(), send_request);

                              log::line( verbose::log, "pending: ", result);
                              
                              return result;
                           };

                           auto callback = [ &state, destination = message.process.ipc, correlation]( auto replies, auto outcome)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::domain::discover::request callback"};

                              detail::advertise::replies( state, replies, outcome);

                              casual::domain::message::discovery::Reply message;
                              message.correlation = correlation;
                              
                              for( auto& reply : replies)
                                 message.content += reply.content;

                              state.multiplex.send( destination, message);
                           };
                           
                           state.coordinate.discovery( 
                              send_requests( state, message),
                              std::move( callback));

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
                           return [ &state]( casual::domain::message::discovery::topology::direct::Explore& message)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::domain::discover::topology::direct::explore"};
                              log::line( verbose::log, "message: ", message);

                              if( state.runlevel > decltype( state.runlevel())::running)
                                 return;

                              auto send_request = []( State& state, auto& message)
                              {
                                 auto result = state.coordinate.discovery.empty_pendings();

                                 casual::domain::message::discovery::Request request;
                                 request.content = std::move( message.content);
                                 request.domain = common::domain::identity();

                                 for( auto& domain : message.domains)
                                 {
                                    if( auto information = state.external.information( domain.id))
                                       if( auto correlation = local::tcp::send( state, information->descriptor, request))
                                          result.emplace_back( correlation, information->descriptor);
                                 }

                                 return result;
                              };
 
                              auto callback = [ &state]( auto replies, auto outcome)
                              {
                                 Trace trace{ "gateway::group::outbound::handle::local::internal::domain::discover::topology::direct::explore callback"};

                                 // We don't need to reply. Only advertise the explored services, if any.
                                 detail::advertise::replies( state, replies, outcome);
                              };

                              state.coordinate.discovery( 
                                 send_request( state, message),
                                 std::move( callback));
                           };

                        }
                     } // topology::direct

                  } // discovery

                  auto connected( State& state)
                  {
                     return [ &state]( const gateway::message::domain::Connected& message)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::internal::domain::connected"};
                        common::log::line( verbose::log, "message: ", message);

                        auto descriptors = state.external.connected( state.directive, message);

                        casual::domain::message::discovery::topology::direct::Update update{ process::handle()};
                        {
                           update.origin = message.domain;

                           const auto information = casual::assertion( algorithm::find( state.external.information(), descriptors.tcp), "failed to find information for descriptor: ", descriptors.tcp);
                           
                           // should we add content
                           if( information->configuration)
                           {
                              update.configured.services = information->configuration.services;
                              update.configured.queues = information->configuration.queues;
                           }
                        }

                        // let the _discovery_ know that the topology has been updated
                        casual::domain::discovery::topology::direct::update( state.multiplex, update);
                     };
                  }

               } // domain

               namespace queue
               {
                  namespace basic
                  {
                     template< typename Message>
                     auto request( State& state)
                     {
                        return [&state]( Message& message)
                        {
                           Trace trace{ "gateway::outbound::local::handle::internal::queue::basic::request"};
                           log::line( verbose::log, "message: ", message);

                           auto lookup = state.lookup.queue( message.name, message.trid);

                           auto route = state::route::Point{ message, lookup.connection, [ &state]( auto& destination)
                           {
                              common::message::reverse::type_t< Message> reply;
                              reply.correlation = destination.correlation;
                              reply.execution = destination.execution;
                              state.multiplex.send( destination.ipc, reply);
                           }};

                           // check if we've has been called with the same correlation id before, hence we are in a
                           // loop between gateways.
                           if( ! lookup.connection || state.route.contains( message.correlation))
                           {
                              log::line( log::category::error, code::casual::invalid_semantics, " failed to look up queue '", message.name, "'");
                              route.error();
                              return;
                           }

                           // notify TM that this "resource" is involved in the branched transaction
                           if( lookup.new_transaction)
                              transaction::involved( message);

                           state.route.add( std::move( route));
                           tcp::send( state, lookup.connection, message);
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
                     auto reply( State& state)
                     {
                        return [&state]( common::message::service::call::Reply& message)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::external::service::call::reply"};
                           log::line( verbose::log, "message: ", message);

                           if( auto route = state.route.consume( message.correlation))
                           {
                              auto pending = state.service.consume( message.correlation);

                              // we unadvertise the service if we get no_entry, and we got 
                              // no connections left for the service
                              if( message.code.result == decltype( message.code.result)::no_entry)
                              {
                                 auto services = state.lookup.remove( route.connection, { pending.service}, {}).services;

                                 if( ! services.empty())
                                 {                                 
                                    common::message::service::Advertise unadvertise{ common::process::handle()};
                                    unadvertise.alias = instance::alias();
                                    unadvertise.services.remove = std::move( services);
                                    state.multiplex.send( ipc::manager::service(), unadvertise);
                                 }
                              }

                              state.multiplex.send( route.destination.ipc, message);

                              internal::service::call::metric( state, std::move( route.destination), pending, message.transaction.trid, message.code.result);
                           }
                           else
                           {
                              log::line( log::category::error, code::casual::internal_correlation, " failed to correlate [", message.correlation, "] reply with a destination - action: ignore");
                              log::line( verbose::log, "state.route.: ", state.route);
                           }
                        };
                     }
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
                           Trace trace{ "gateway::group::outbound::handle::local::external::conversation::connect::reply"};
                           log::line( verbose::log, "message: ", message);

                           if( auto found = algorithm::find( state.route.points(), message.correlation))
                           {
                              log::line( verbose::log, "found: ", *found);
                              state.multiplex.send( found->destination.ipc, message);
                              
                              if( message.code.result != decltype( message.code.result)::absent)
                                 state.route.remove( message.correlation);
                           }
                           else
                           {
                              log::line( log::category::error, code::casual::internal_correlation, " failed to correlate external::conversation::connect::reply [", message.correlation, "] - action: ignore");
                              log::line( verbose::log, "state.route: ", state.route);
                           }

                        };
                     }
                  } // connect

                  auto send( State& state)
                  {
                     return [&state]( common::message::conversation::callee::Send& message)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::external::conversation::send"};
                        log::line( verbose::log, "message: ", message);

                        if( auto found = algorithm::find( state.route.points(), message.correlation))
                        {
                           state.multiplex.send( found->destination.ipc, message);

                           if( message.code.result != decltype( message.code.result)::absent)
                                 state.route.remove( message.correlation);
                        }
                        else
                        {
                           log::line( log::category::error, code::casual::internal_correlation, " failed to correlate external::conversation::send [", message.correlation, "] - action: ignore");
                           log::line( verbose::log, "state.route: ", state.route);
                        }

                     };
                  }
               } // conversation

               namespace queue
               {
                  namespace basic
                  {
                     template< typename Message>
                     auto reply( State& state)
                     {
                        return [&state]( Message& message)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::external::queue::basic::reply"};
                           log::line( verbose::log, "message: ", message);

                           if( auto point = state.route.consume( message.correlation))
                           {
                              state.multiplex.send( point.destination.ipc, message);
                           }
                           else
                              log::line( log::category::error, code::casual::internal_correlation, " failed to correlate [", message.correlation, "] reply with a destination - action: ignore");
                        };
                     }
                  } // basic

                  namespace enqueue
                  {
                     auto reply = basic::reply< casual::queue::ipc::message::group::enqueue::Reply>;
                  } // enqueue

                  namespace dequeue
                  {
                     auto reply = basic::reply< casual::queue::ipc::message::group::dequeue::Reply>;
                  } // dequeue
               } // queue

               namespace transaction
               {
                  namespace resource
                  {
                     namespace prepare
                     {
                        auto reply( State& state)
                        {
                           return [ &state]( common::message::transaction::resource::prepare::Reply& message, strong::socket::id descriptor)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::external::transaction::resource::prepare::reply"};
                              log::line( verbose::log, "message: ", message);

                              // if the prepare is a _read-only_, the transaction is done for the connection
                              if( message.state == decltype( message.state)::read_only)
                                 state.lookup.remove( common::transaction::id::range::global( message.trid), descriptor);

                              state.coordinate.transaction.prepare( message);
                           };
                        }
                           
                     } // prepare

                     namespace commit
                     {
                        auto reply( State& state)
                        {
                           return [ &state]( common::message::transaction::resource::commit::Reply& message, strong::socket::id descriptor)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::external::transaction::resource::commit::reply"};
                              log::line( verbose::log, "message: ", message);

                              state.lookup.remove( common::transaction::id::range::global( message.trid), descriptor);
                              state.coordinate.transaction.commit( message);
                           };
                        }

                     } // commit

                     namespace rollback
                     {
                        auto reply( State& state)
                        {
                           return [ &state]( common::message::transaction::resource::rollback::Reply& message, strong::socket::id descriptor)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::external::transaction::resource::rollback::reply"};
                              log::line( verbose::log, "message: ", message);

                              state.lookup.remove( common::transaction::id::range::global( message.trid), descriptor);
                              state.coordinate.transaction.rollback( message);
                           };
                        }
                     } // rollback

                  } // resource
               } // transaction

               namespace domain::discovery
               {
                  auto reply( State& state)
                  {
                     return [&state]( casual::domain::message::discovery::Reply&& message)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::external::domain::discover::reply"};
                        log::line( verbose::log, "message: ", message);

                        // increase hops for all services.
                        for( auto& service : message.content.services)
                           ++service.property.hops;

                        state.coordinate.discovery( std::move( message));                         
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

                           // make sure to set who actually is updated.
                           if( auto information = state.external.information( descriptor))
                              message.origin = information->domain;

                           casual::domain::discovery::topology::implicit::update( state.multiplex, message);
                        };
                     }
                  } // topology

               } // domain::discovery
            } // external


         } // <unnamed>
      } // local
         

      internal_handler internal( State& state)
      {
         casual::domain::discovery::provider::registration( casual::domain::discovery::provider::Ability::discover);
         
         return internal_handler{

            local::internal::domain::connected( state),

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

            // conversation
            local::external::conversation::connect::reply( state),
            local::external::conversation::send( state),

            // queue
            local::external::queue::enqueue::reply( state),
            local::external::queue::dequeue::reply( state),

            // transaction
            local::external::transaction::resource::prepare::reply( state),
            local::external::transaction::resource::commit::reply( state),
            local::external::transaction::resource::rollback::reply( state),

            // discover
            local::external::domain::discovery::reply( state),
            local::external::domain::discovery::topology::update( state)
         };
      }

      void unadvertise( State& state, state::lookup::Resources resources)
      {
         Trace trace{ "gateway::group::outbound::handle::unadvertise"};
         log::line( verbose::log, "resources: ", resources);

         if( ! resources.services.empty())
         {
            common::message::service::concurrent::Advertise request{ common::process::handle()};
            request.alias = instance::alias();
            request.services.remove = std::move( resources.services);
            state.multiplex.send( ipc::manager::service(), request);
         }

         if( ! resources.queues.empty())
         {
            casual::queue::ipc::message::Advertise request{ common::process::handle()};
            request.queues.remove = std::move( resources.queues);
            // this throws if queue-manager is absent.
            // TODO send on instance::optional::Device should not throw?
            common::exception::guard( [ &]()
            {
               state.multiplex.send( ipc::manager::optional::queue(), request);
            });
            
         }
      }

      namespace connection
      {
         message::outbound::connection::Lost lost( State& state, strong::socket::id descriptor)
         {
            Trace trace{ "gateway::group::outbound::handle::connection::lost"};
            log::line( verbose::log, "descriptor: ", descriptor);

            // unadvertise all 'orphanage' services and queues, if any.
            handle::unadvertise( state, state.lookup.remove( descriptor));

            // take care of aggregated/coordinated replies, if any.
            state.coordinate.failed( descriptor);

            // extract all state associated with the descriptor
            auto extracted = state.failed( descriptor);
            log::line( verbose::log, "extracted: ", extracted);

            if( ! extracted.empty())
            {
               log::line( log::category::error, code::casual::communication_unavailable, " lost connection - address: ", extracted.information.configuration.address, ", domain: ", extracted.information.domain);
               log::line( log::category::verbose::error, "extracted: ", extracted);

               auto error_reply = []( auto& point){ point.error();};

               // try to send 'error-replies' to all 'routes', if any.
               algorithm::for_each( extracted.routes, error_reply);
            }

            return { std::move( extracted.information.configuration), std::move( extracted.information.domain)};
         }

         void disconnect( State& state, common::strong::socket::id descriptor)
         {
            Trace trace{ "gateway::group::outbound::handle::connection::disconnect"};
            log::line( verbose::log, "descriptor: ", descriptor);

            // unadvertise all 'orphanage' services and queues, if any.
            handle::unadvertise( state, state.lookup.remove( descriptor));

            state.disconnecting.push_back( descriptor);
         }
         
      } // connection

      void idle( State& state)
      {
         Trace trace{ "gateway::group::outbound::handle::idle"};

         // we need to check metric, we don't know when we're about to be called again.
         state.service.force_metric( state, []( auto& state, auto& message)
         {
            state.multiplex.send( ipc::manager::service(), message);
         });
      }

      void shutdown( State& state)
      {
         Trace trace{ "gateway::group::outbound::handle::shutdown"};

         state.runlevel = state::Runlevel::shutdown;

         // unadvertise all resources
         handle::unadvertise( state, state.lookup.resources());

         // send metric for good measure
         state.service.force_metric( state, []( auto& state, auto& message)
         {
            state.multiplex.send( ipc::manager::service(), message);
         });

         for( auto descriptor : state.external.external_descriptors())
            handle::connection::disconnect( state, descriptor);

         log::line( verbose::log, "state: ", state);
      }

      void abort( State& state)
      {
         Trace trace{ "gateway::group::outbound::handle::abort"};
         log::line( log::category::verbose::error, "abort - state: ", state);

         state.runlevel = state::Runlevel::error;

         handle::unadvertise( state, state.lookup.resources());

         state.external.clear( state.directive);

         auto error_reply = []( auto& point){ point.error();};
         algorithm::for_each( state.route.consume(), error_reply);
      }

   } // gateway::group::outbound::handle
} // casual