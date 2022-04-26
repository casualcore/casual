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
#include "common/message/handle.h"
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
               strong::correlation::id send( State& state, strong::file::descriptor::id descriptor, M&& message)
               {
                  if( auto connection = state.external.connection( descriptor))
                  {
                     try
                     {
                        return connection->send( state.directive, std::forward< M>( message));
                     }
                     catch( ...)
                     {
                        const auto error = exception::capture();

                        auto lost = connection::lost( state, descriptor);

                        if( error.code() != code::casual::communication_unavailable)
                           log::line( log::category::error, error, " send failed to remote: ", lost.remote, " - action: remove connection");

                        // we 'lost' the connection in some way - we put a connection::Lost on our own ipc-device, and handle it
                        // later (and differently depending on if we're 'regular' or 'reversed')
                        communication::ipc::inbound::device().push( lost);
                     }
                  }
                  else
                  {
                     log::line( log::category::error, code::casual::internal_correlation, " failed to correlate descriptor: ", descriptor);
                     log::line( log::category::verbose::error, "state: ", state);
                  }

                  return {};
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

                        ipc::flush::send( 
                           ipc::manager::transaction(),
                           common::message::transaction::resource::external::involved::create( message));
                     }
                  }

                  namespace resource
                  {    
                     namespace basic
                     {
                        template< typename Message>
                        auto request( State& state)
                        {
                           return [&state]( Message& message)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::transaction::resource::basic::request"};
                              log::line( verbose::log, "message: ", message);

                              auto descriptor = state.lookup.connection( message.trid);

                              // add the route and the error callback
                              state.route.add( std::move( message), descriptor, [ &state, rm = message.resource, trid = message.trid]( auto& destination)
                              {
                                 common::message::reverse::type_t< Message> reply;
                                 reply.correlation = destination.correlation;
                                 reply.execution = destination.execution;
                                 reply.resource = rm;
                                 reply.trid = trid;
                                 reply.state = decltype( reply.state)::resource_error; // is this the best error code?

                                 log::line( verbose::log, "reply: ", reply);
                                 state.multiplex.send( destination.ipc, reply);
                              });

                              tcp::send( state, descriptor, message);
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
                     } // commit
                  } // resource
               } // transaction

               namespace service
               {
                  namespace call
                  {
                     auto metric( State& state, const state::route::Destination& destination, 
                        const state::service::Pending::Call& call, const common::transaction::ID& trid, code::xatmi code)
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
                        state.service.maybe_metric( [ &state]( auto& message)
                        {
                           state.multiplex.send( ipc::manager::service(), message);
                        });
                     }                  

                     auto request( State& state)
                     {
                        return [&state]( common::message::service::call::callee::Request& message)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::internal::service::call::request"};
                           log::line( verbose::log, "message: ", message);

                           auto [ lookup, involved] = state.lookup.service( message.service.name, message.trid);
                           log::line( verbose::log, "lookup: ", lookup);

                           state.service.add( message);

                           auto create_error_callback = []( auto& state, auto& message) -> state::route::error::callback::type
                           {
                              if( message.flags.exist( common::message::service::call::request::Flag::no_reply))
                                 return {};

                              return [ &state, trid = message.trid]( auto& destination)
                              {
                                 common::message::service::call::Reply reply;
                                 reply.correlation = destination.correlation;
                                 reply.execution = destination.execution;
                                 reply.transaction.trid = trid;
                                 reply.code.result = decltype( reply.code.result)::system;

                                 log::line( verbose::log, "reply: ", reply);
                                 state.multiplex.send( destination.ipc, reply);

                                 service::call::metric( state, destination, state.service.consume( destination.correlation), trid, reply.code.result);
                              };
                           };

                           auto route = state::route::Point{ message, lookup.connection, create_error_callback( state, message)};

                           // check if we've has been called with the same correlation id before, hence we are in a
                           // loop between gateways.
                           if( state.route.contains( message.correlation))
                           {
                              log::line( log, code::casual::invalid_semantics, " a call with the same correlation id is in flight - ", message.correlation, " - action: reply with ", code::xatmi::no_entry);
                              if( route.callback)
                                 route.error();
                              return;
                           }

                           if( ! lookup.connection)
                           {
                              log::line( log::category::error, code::casual::invalid_semantics, " failed to look up service '", message.service.name, "' - action: reply with ", code::xatmi::system);

                              if( route.callback)
                                 route.error();
                              return;
                           }

                           // set the branched trid for the message, if any.
                           std::exchange( message.trid, lookup.trid);

                           // notify TM that we act as a "resource" and are involved in the transaction
                           if( involved)
                              transaction::involved( message);
                              
                           if( ! message.flags.exist( common::message::service::call::request::Flag::no_reply))
                              state.route.add( std::move( route));

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

                           auto [ lookup, involved] = state.lookup.service( message.service.name, message.trid);

                           message.trid = lookup.trid;

                           // notify TM that this "resource" is involved in the branched transaction
                           if( involved)
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
                           auto replies( State& state, R& replies, const O& outcome)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::domain::discover::detail::advertise::replies"};

                              auto advertise_connection = [&state]( auto& reply, auto connection)
                              {

                                 auto advertise = state.lookup.add( 
                                    connection, 
                                    algorithm::transform( reply.content.services, []( auto& service){ return state::lookup::Resource{ service.name, service.property.hops};}), 
                                    algorithm::transform( reply.content.queues, []( auto& service){ return state::lookup::Resource{ service.name, 1};}));


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

                              for( auto& reply : replies)
                                 if( auto found = algorithm::find( outcome, reply.correlation))
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

                              state.multiplex.send( message.process.ipc, common::message::reverse::type( message, common::process::handle()));
                              return;
                           }

                           // make sure we get unique correlations for the coordination
                           auto correlation = std::exchange( message.correlation, {});

                           auto send_requests = []( State& state, auto& message)
                           {
                              auto result = state.coordinate.discovery.empty_pendings();

                              algorithm::for_each( state.external.connections(), [&state, &result, &message]( auto& connection)
                              {
                                 auto descriptor = connection.descriptor();

                                 if( algorithm::find( state.disconnecting, descriptor))
                                    return;

                                 // check if the connection is the same domain that instigated the discovery
                                 if( auto found = state.external.information( descriptor))
                                    if( found->domain.id == message.domain.id)
                                       return;

                                 // TODO: optional-send
                                 if( auto correlation = connection.send( state.directive, message))
                                    result.emplace_back( correlation, descriptor);
                              });

                              log::line( verbose::log, "pending: ", result);
                              
                              return result;
                           };

                           auto callback = [&state, destination = message.process.ipc, correlation]( auto replies, auto outcome)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::domain::discover::request callback"};

                              detail::advertise::replies( state, replies, outcome);

                              casual::domain::message::discovery::Reply message{ common::process::handle()};
                              message.correlation = correlation;
                              
                              message.content = algorithm::accumulate( replies, decltype( message.content){}, []( auto result, auto& reply)
                              {
                                 return result + reply.content;
                              });

                              state.multiplex.send( destination, message);
                           };
                           
                           state.coordinate.discovery( 
                              send_requests( state, message),
                              std::move( callback)
                           );

                        };
                     }

                  } // discovery

      
                  auto connected( State& state)
                  {
                     return [&state]( const gateway::message::domain::Connected& message)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::internal::domain::connected"};
                        common::log::line( verbose::log, "message: ", message);

                        auto descriptor = state.external.connected( state.directive, message);

                        if( auto information = algorithm::find( state.external.information(), descriptor))
                        {
                           // should we do discovery
                           if( information->configuration)
                           {
                              auto send_request = []( auto& state, auto& message, auto& configuration, auto descriptor)
                              {
                                 auto result = state.coordinate.discovery.empty_pendings();

                                 casual::domain::message::discovery::Request request{ common::process::handle()};
                                 request.correlation = message.correlation;
                                 request.domain = common::domain::identity();
                                 request.content.services = configuration.services;
                                 request.content.queues = configuration.queues;
                                 result.emplace_back( tcp::send( state, descriptor, request), descriptor);

                                 return result;
                              };

                              auto callback = [&state]( auto&& replies, auto&& outcome)
                              {
                                 Trace trace{ "gateway::group::outbound::handle::local::internal::domain::connected callback"};

                                 // since we've instigated the request, we just advertise and we're done.
                                 discovery::detail::advertise::replies( state, replies, outcome);
                              };
                              
                              state.coordinate.discovery( 
                                 send_request( state, message, information->configuration, descriptor),
                                 std::move( callback)
                              );
                           }

                           // let the _discovery_ know that the topology has been updated
                           casual::domain::discovery::topology::update();

                           log::line( verbose::log, "information: ", *information);
                        }
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

                           auto [ lookup, involved] = state.lookup.queue( message.name, message.trid);

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
                              log::line( log::category::error, code::casual::invalid_semantics, " failed to look up queue '", message.name);
                              route.error();
                              return;
                           }

                           message.trid = lookup.trid;

                           // notify TM that this "resource" is involved in the branched transaction
                           if( involved)
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
                     return [&state]( const gateway::message::domain::disconnect::Request& message)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::external::disconnect::request"};
                        log::line( verbose::log, "message: ", message);

                        auto descriptor = state.external.last();

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

                              // get the internal "un-branched" trid
                              message.transaction.trid = state.lookup.internal( message.transaction.trid);

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
                     namespace detail
                     {
                        template< typename M>
                        void send( State& state, M&& message)
                        {
                           if( auto point = state.route.consume( message.correlation))
                           {
                              message.process = process::handle();
                              state.multiplex.send( point.destination.ipc, message);
                           }
                           else
                              log::line( log::category::error, code::casual::internal_correlation, " failed to correlate [", message.correlation, "] reply with a destination - action: ignore");
                        }
                     } // detail
                     namespace prepare
                     {
                        auto reply( State& state)
                        {
                           return [&state]( common::message::transaction::resource::prepare::Reply& message)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::external::transaction::resource::prepare::reply"};
                              log::line( verbose::log, "message: ", message);

                              // We have register the external branch with the TM so it knows about it, hence
                              // can communicate directly with the external branched trid

                              // this trid is NOT done, we have to wait until commit or rollback

                              detail::send( state, message);
                           };
                        }
                        
                     } // prepare
                     namespace commit
                     {
                        auto reply( State& state)
                        {
                           return [&state]( common::message::transaction::resource::commit::Reply& message)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::external::transaction::resource::commit::reply"};
                              log::line( verbose::log, "message: ", message);

                              // We have register the external branch with the TM so it knows about it, hence
                              // can communicate directly with the external branched trid

                              // this trid is done, remove it from the state
                              state.lookup.remove( message.trid);

                              detail::send( state, message);
                           };
                        }

                     } // commit
                     namespace rollback
                     {
                        auto reply( State& state)
                        {
                           return [&state]( common::message::transaction::resource::rollback::Reply& message)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::external::transaction::resource::rollback::reply"};
                              log::line( verbose::log, "message: ", message);

                              // We have register the external branch with the TM so it knows about it, hence
                              // can communicate directly with the external branched trid

                              // this trid is done, remove it from the state
                              state.lookup.remove( message.trid);

                              detail::send( state, message);
                           };
                        }

                     } // commit
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
                     auto update()
                     {
                        return []( casual::domain::message::discovery::topology::Update&& message)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::external::domain::discover::topology::update"};
                           log::line( verbose::log, "message: ", message);

                           // no need to send it if we've seen this message before
                           if( algorithm::find( message.domains, common::domain::identity()))
                              return;

                           casual::domain::discovery::topology::update( message);
                        };
                     }
                  } // topology

               } // domain::discovery
            } // external


         } // <unnamed>
      } // local
         

      internal_handler internal( State& state)
      {
         casual::domain::discovery::provider::registration( casual::domain::discovery::provider::Ability::discover_external);
         
         return {
            common::message::handle::defaults( ipc::inbound()),
            common::message::internal::dump::state::handle( state),

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
         };
      }

      external_handler external( State& state)
      {
         return {
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
            local::external::domain::discovery::topology::update()
         };
      }

      void unadvertise( state::lookup::Resources resources)
      {
         Trace trace{ "gateway::group::outbound::handle::unadvertise"};
         log::line( verbose::log, "resources: ", resources);

         if( ! resources.services.empty())
         {
            common::message::service::concurrent::Advertise request{ common::process::handle()};
            request.alias = instance::alias();
            request.services.remove = std::move( resources.services);
            ipc::flush::send( ipc::manager::service(), request);
         }

         if( ! resources.queues.empty())
         {
            casual::queue::ipc::message::Advertise request{ common::process::handle()};
            request.queues.remove = std::move( resources.queues);
            ipc::flush::send( ipc::manager::optional::queue(), request);
         }
      }

      namespace connection
      {
         message::outbound::connection::Lost lost( State& state, strong::file::descriptor::id descriptor)
         {
            Trace trace{ "gateway::group::outbound::handle::connection::lost"};
            log::line( verbose::log, "descriptor: ", descriptor);

            // unadvertise all 'orphanage' services and queues, if any.
            handle::unadvertise( state.lookup.remove( descriptor));

            // take care of aggregated replies, if any.
            state.coordinate.discovery.failed( descriptor);

            auto error_reply = []( auto& point){ point.error();};

            // consume routs associated with the 'connection', and try to send 'error-replies'
            algorithm::for_each( state.route.consume( descriptor), error_reply);

            // connection might have been in 'disconnecting phase'
            algorithm::container::trim( state.disconnecting, algorithm::remove( state.disconnecting, descriptor));

            // remove the information about the 'connection'.
            auto information = state.external.remove( state.directive, descriptor);
            return { std::move( information.configuration), std::move( information.domain)};
         }

         void disconnect( State& state, common::strong::file::descriptor::id descriptor)
         {
            Trace trace{ "gateway::group::outbound::handle::connection::disconnect"};
            log::line( verbose::log, "descriptor: ", descriptor);

            // unadvertise all 'orphanage' services and queues, if any.
            handle::unadvertise( state.lookup.remove( descriptor));

            state.disconnecting.push_back( descriptor);
         }
         
      } // connection

      void idle( State& state)
      {
         Trace trace{ "gateway::group::outbound::handle::idle"};

         // we need to check metric, we don't know when we're about to be called again.
         state.service.force_metric( []( auto& message)
         {
            ipc::flush::optional::send( ipc::manager::service(), message);
         });
      }

      void shutdown( State& state)
      {
         Trace trace{ "gateway::group::outbound::handle::shutdown"};

         state.runlevel = state::Runlevel::shutdown;

         // unadvertise all resources
         handle::unadvertise( state.lookup.resources());

         // send metric for good measure
         state.service.force_metric( []( auto& message)
         {
            ipc::flush::optional::send( ipc::manager::service(), message);
         });

         for( auto descriptor : state.external.descriptors())
            handle::connection::disconnect( state, descriptor);

         log::line( verbose::log, "state: ", state);
      }

      void abort( State& state)
      {
         Trace trace{ "gateway::group::outbound::handle::abort"};
         log::line( log::category::verbose::error, "abort - state: ", state);

         state.runlevel = state::Runlevel::error;

         handle::unadvertise( state.lookup.resources());

         state.external.clear( state.directive);

         auto error_reply = []( auto& point){ point.error();};
         algorithm::for_each( state.route.consume(), error_reply);
      }

   } // gateway::group::outbound::handle
} // casual