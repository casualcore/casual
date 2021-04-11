//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/outbound/handle.h"
#include "gateway/group/outbound/error/reply.h"
#include "gateway/group/tcp.h"
#include "gateway/group/ipc.h"

#include "gateway/message.h"
#include "gateway/common.h"

#include "domain/discovery/api.h"

#include "common/communication/instance.h"
#include "common/message/handle.h"
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
               auto send( State& state, strong::file::descriptor::id descriptor, M&& message)
               {
                  return group::tcp::send( state, descriptor, std::forward< M>( message), &handle::connection::lost);
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

                              tcp::send( state, descriptor, message);
                              state.route.message.add( std::move( message), descriptor);
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
                     namespace reply
                     {
                        auto send( State& state, state::route::service::Point destination, const common::message::service::call::Reply& message)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::internal::service::call::reply::send"};

                           state.route.service.metric.metrics.push_back( [&]()
                           {
                              common::message::event::service::Metric metric;
                              
                              metric.process = common::process::handle();
                              metric.correlation = message.correlation;
                              metric.execution = message.execution;
                              metric.service = std::move( destination.service);
                              metric.parent = std::move( destination.parent);
                              metric.type = decltype( metric.type)::concurrent;
                              
                              metric.trid = message.transaction.trid;
                              metric.start = destination.start;
                              metric.end = platform::time::clock::type::now();

                              metric.code = message.code.result;

                              return metric;
                           }());

                           ipc::flush::optional::send( destination.process.ipc, message);

                           // send service metrics if we don't have any more in-flight call request (this one
                           // was the last, or only) OR we've accumulated enough metrics for a batch update
                           if( state.route.service.message.empty() || state.route.service.metric.metrics.size() >= platform::batch::gateway::metrics)
                           {
                              ipc::flush::optional::send( ipc::manager::service(), state.route.service.metric);
                              state.route.service.metric.metrics.clear();
                           }
                        }
                     } // reply

                     auto request( State& state)
                     {
                        return [&state]( common::message::service::call::callee::Request& message)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::internal::service::call::request"};
                           log::line( verbose::log, "message: ", message);

                           auto now = platform::time::clock::type::now();

                           auto [ lookup, involved] = state.lookup.service( message.service.name, message.trid);

                           log::line( verbose::log, "lookup: ", lookup);

                           auto route = state::route::service::Point{
                              message.correlation,
                              message.process,
                              // we keep the requested if provided. Will be used for "metric-ack" to service-manager later.
                              message.service.requested.value_or( message.service.name),
                              message.parent,
                              now,
                              lookup.connection};

                           auto origin_trid = std::exchange( message.trid, lookup.trid);

                           
                           if( ! lookup.connection)
                           {
                              log::line( log::category::error, code::casual::invalid_semantics, " failed to look up service '", message.service.name, "' - action: reply with ", code::xatmi::system);

                              // we can't send error reply if the caller using 'fire and forget'.
                              if( message.flags.exist( common::message::service::call::request::Flag::no_reply))
                                 return;

                              auto reply = common::message::reverse::type( message);
                              reply.code.result = code::xatmi::system;
                              reply.transaction.trid = origin_trid;

                              service::call::reply::send( state, std::move( route), reply);
                              return;
                           }

                           // notify TM that we act as a "resource" and are involved in the transaction
                           if( involved)
                              transaction::involved( message);
                              
                           if( ! message.flags.exist( common::message::service::call::request::Flag::no_reply))
                              state.route.service.message.add( std::move( route));

                           tcp::send( state, lookup.connection, message);
                        };
                     }

                  } // call

                  namespace conversation
                  {
                     namespace connect
                     {
                        auto request( State& state)
                        {
                           return [&state]( common::message::conversation::connect::callee::Request& message)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::service::conversation::connect::request"};
                              log::line( verbose::log, "message: ", message);

                              auto [ lookup, involved] = state.lookup.service( message.service.name, message.trid);

                              message.trid = lookup.trid;

                              // notify TM that this "resource" is involved in the branched transaction
                              if( involved)
                                 transaction::involved( message);

                              state.route.message.add( message, lookup.connection);
                              tcp::send( state, lookup.connection, message);
                           };
                        }

                     } // connect

                  } // conversation
               } // service

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
                                 auto get_name = []( auto& resource){ return resource.name;};

                                 auto advertise = state.lookup.add( 
                                    connection, 
                                    algorithm::transform( reply.content.services, get_name), 
                                    algorithm::transform( reply.content.queues, get_name));


                                 auto equal_name = []( auto& lhs, auto& rhs){ return lhs.name == rhs;};

                                 // we need to intersect whit the actual 'resource' information with what the state says we should advertise.
                                 // (we do not know the _information_, and don't really care)

                                 if( auto services = std::get< 0>( algorithm::intersection( reply.content.services, advertise.services, equal_name)))
                                 {
                                    common::message::service::concurrent::Advertise request{ common::process::handle()};
                                    request.alias = instance::alias();
                                    request.order = state.order;
                                    algorithm::copy( services, request.services.add);

                                    ipc::flush::send( ipc::manager::service(), request);
                                 }

                                 if( auto queues = std::get< 0>( algorithm::intersection( reply.content.queues, advertise.queues, equal_name)))
                                 {
                                    casual::queue::ipc::message::Advertise request{ common::process::handle()};
                                    request.order = state.order;
                                    request.queues.add = algorithm::transform( queues, []( auto& queue)
                                    {
                                       return casual::queue::ipc::message::advertise::Queue{ queue.name, queue.retries};
                                    });

                                    ipc::flush::send( ipc::manager::optional::queue(), request);
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
                        return [&state]( gateway::message::domain::discovery::Request& message)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::internal::domain::discover::request"};
                           log::line( verbose::log, "message: ", message);

                           if( state.runlevel > decltype( state.runlevel())::running)
                           {
                              log::line( log, "outbound is in shutdown mode - action: reply with empty discovery");

                              ipc::flush::optional::send( 
                                 message.process.ipc, 
                                 common::message::reverse::type( message, process::handle()));
                              return;
                           }

                           // make sure we get unique correlations for the coordination
                           auto correlation = std::exchange( message.correlation, {});

                           auto send_requests = []( State& state, auto& message)
                           {
                              auto result = state.coordinate.discovery.empty_pendings();

                              algorithm::for_each( state.external.connections, [&state, &result, &message]( auto& connection)
                              {
                                 auto descriptor = connection.descriptor();

                                 if( algorithm::find( state.disconnecting, descriptor))
                                    return;

                                 // check if the connection is the same domain that instigated the discovery
                                 if( auto found = algorithm::find( state.external.information, descriptor))
                                    if( found->domain.id == message.domain.id)
                                       return;

                                 // TODO: optional-send
                                 if( auto correlation = connection.send( state, message))
                                    result.emplace_back( correlation, descriptor);
                              });

                              log::line( verbose::log, "pending: ", result);
                              
                              return result;
                           };

                           auto callback = [&state, destination = message.process.ipc, correlation]( auto replies, auto outcome)
                           {
                              Trace trace{ "gateway::group::outbound::handle::local::internal::domain::discover::request callback"};

                              detail::advertise::replies( state, replies, outcome);

                              gateway::message::domain::discovery::Reply message{ process::handle()};
                              message.correlation = correlation;
                              
                              message.content = algorithm::accumulate( replies, decltype( message.content){}, []( auto result, auto& reply)
                              {
                                 return result + reply.content;
                              });

                              ipc::flush::optional::send( destination, message);
                           };
                           
                           state.coordinate.discovery( 
                              send_requests( state, message),
                              std::move( callback)
                           );

                        };
                     }
                  } // discovery

                  namespace rediscover
                  {
                     auto request( State& state)
                     {
                        return [&state]( casual::domain::message::discovery::rediscovery::Request& message)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::internal::domain::rediscover::request"};
                           log::line( verbose::log, "message: ", message);

                           if( state.runlevel > decltype( state.runlevel())::running)
                           {
                              ipc::flush::optional::send( 
                                 message.process.ipc, 
                                 common::message::reverse::type( message));
                              return;
                           }

                           auto pending = algorithm::accumulate( state.external.information, state.coordinate.discovery.empty_pendings(), [&state]( auto result, auto& information)
                           {
                              if( ( information.configuration.services.empty() 
                                 && information.configuration.queues.empty())
                                 || algorithm::find( state.disconnecting, information.connection))
                                 return result;

                              gateway::message::domain::discovery::Request request;
                              request.domain = common::domain::identity();
                              request.content.services = information.configuration.services;
                              request.content.queues = information.configuration.queues;

                              result.emplace_back( local::tcp::send( state, information.connection, request), information.connection);
                              return result;
                           });


                           state.coordinate.discovery( std::move( pending), [&state, correlation = message.correlation, ipc = message.process.ipc]( auto&& replies, auto&& outcome)
                           { 
                              Trace trace{ "gateway::group::outbound::handle::local::internal::domain::rediscover::request callback"};

                              // clears and unadvertise all 'resources', if any.
                              handle::unadvertise( state.lookup.clear());

                              // advertise the newly found, if any.
                              discovery::detail::advertise::replies( state, replies, outcome);

                              casual::domain::message::discovery::rediscovery::Reply reply;
                              reply.correlation = correlation;
                              ipc::flush::optional::send( ipc, reply);
                           });

                        };
                     }
                  } // rediscover

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

                           
                           if( ! lookup.connection)
                           {
                              log::line( log::category::error, code::casual::invalid_semantics, " failed to look up queue '", message.name);
                              auto reply = common::message::reverse::type( message);
                              ipc::flush::optional::send( message.process.ipc, reply);
                              return;
                           }

                           message.trid = lookup.trid;

                           // notify TM that this "resource" is involved in the branched transaction
                           if( involved)
                              transaction::involved( message);

                           state.route.message.add( message, lookup.connection);
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
               namespace connect
               {
                  auto reply( State& state)
                  {
                     return [&state]( const gateway::message::domain::connect::Reply& message)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::external::connect::reply"};
                        log::line( verbose::log, "message: ", message);

                        auto destination = state.route.message.consume( message.correlation);

                        if( ! destination)
                        {
                           log::line( log::category::error, code::casual::internal_correlation, " failed to correlate [", message.correlation, "] reply with a destination - action: ignore");
                           return;
                        }

                        if( algorithm::none_of( gateway::message::domain::protocol::versions, predicate::value::equal( message.version)))
                           code::raise::error( code::casual::invalid_version, "invalid protocol version: ", message.version);

                        if( auto information = algorithm::find( state.external.information, destination.connection))
                        {
                           information->domain = message.domain;

                           // should we do discovery
                           if( information->configuration)
                           {
                              auto send_request = []( auto& state, auto& message, auto& configuration, auto& destination)
                              {
                                 auto result = state.coordinate.discovery.empty_pendings();

                                 gateway::message::domain::discovery::Request request{ common::process::handle()};
                                 request.correlation = message.correlation;
                                 request.domain = common::domain::identity();
                                 request.content.services = configuration.services;
                                 request.content.queues = configuration.queues;
                                 result.emplace_back( tcp::send( state, destination.connection, request), destination.connection);

                                 return result;
                              };

                              auto callback = [&state]( auto&& replies, auto&& outcome)
                              {
                                 Trace trace{ "gateway::group::outbound::handle::local::external::connect::reply callback"};

                                 // since we've instigated the request, we just advertise and we're done.
                                 internal::domain::discovery::detail::advertise::replies( state, replies, outcome);
                              };
                              
                              state.coordinate.discovery( 
                                 send_request( state, message, information->configuration, destination),
                                 std::move( callback)
                              );
                           }

                           // register to domain discovery
                           casual::domain::discovery::outbound::registration( common::process::handle());

                           log::line( verbose::log, "information: ", *information);
                        }
                     };
                  }
               } // connect

               namespace disconnect
               {
                  auto request( State& state)
                  {
                     return [&state]( const gateway::message::domain::disconnect::Request& message)
                     {
                        Trace trace{ "gateway::group::outbound::handle::local::external::disconnect::request"};
                        log::line( verbose::log, "message: ", message);

                        auto descriptor = state.external.last;

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

                           if( auto destination = state.route.service.message.consume( message.correlation))
                           {
                              // we unadvertise the service if we get no_entry, and we got 
                              // no connections left for the service
                              if( message.code.result == decltype( message.code.result)::no_entry)
                              {
                                 auto services = state.lookup.remove( destination.connection, { destination.service}, {}).services;

                                 if( ! services.empty())
                                 {                                 
                                    common::message::service::Advertise unadvertise{ common::process::handle()};
                                    unadvertise.alias = instance::alias();
                                    unadvertise.services.remove = std::move( services);
                                    ipc::flush::optional::send( ipc::manager::service(), unadvertise);
                                 }
                              }

                              // get the internal "un-branched" trid
                              message.transaction.trid = state.lookup.internal( message.transaction.trid);

                              internal::service::call::reply::send( state, std::move( destination), message);
                           }
                           else
                           {
                              log::line( log::category::error, code::casual::internal_correlation, " failed to correlate [", message.correlation, "] reply with a destination - action: ignore");
                              log::line( verbose::log, "state.route.service.message: ", state.route.service.message);
                           }
                        };
                     }

                  } // call

               } // service

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

                           if( auto destination = state.route.message.consume( message.correlation))
                           {
                              ipc::flush::optional::send( destination.process.ipc, message);
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
                           if( auto destination = state.route.message.consume( message.correlation))
                           {
                              message.process = process::handle();
                              ipc::flush::optional::send( destination.process.ipc, message);
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

                              // this trid is done, remove it from the state
                              state.lookup.remove( message.trid);

                              detail::send( state, message);
                           };
                        }

                     } // commit
                  } // resource
               } // transaction

               namespace domain
               {
                  namespace discovery
                  {
                     auto reply( State& state)
                     {
                        return [&state]( gateway::message::domain::discovery::Reply& message)
                        {
                           Trace trace{ "gateway::group::outbound::handle::local::external::domain::discover::reply"};
                           log::line( verbose::log, "message: ", message);

                           state.coordinate.discovery( std::move( message));                         
                        };
                     }
                  } // discover
               } // domain
  
            } // external


         } // <unnamed>
      } // local
         

      internal_handler internal( State& state)
      {
         return {
            common::message::handle::defaults( ipc::inbound()),

            // service
            local::internal::service::call::request( state),
            local::internal::service::conversation::connect::request( state),
            
            // queue
            local::internal::queue::dequeue::request( state),
            local::internal::queue::enqueue::request( state),

            // transaction
            local::internal::transaction::resource::prepare::request( state),
            local::internal::transaction::resource::commit::request( state),
            local::internal::transaction::resource::rollback::request( state),

            // discover
            local::internal::domain::discovery::request( state),
            local::internal::domain::rediscover::request( state),
         };
      }

      external_handler external( State& state)
      {
         return {
            local::external::connect::reply( state),
            local::external::disconnect::request( state),
            
            // service
            local::external::service::call::reply( state),

            // queue
            local::external::queue::enqueue::reply( state),
            local::external::queue::dequeue::reply( state),

            // transaction
            local::external::transaction::resource::prepare::reply( state),
            local::external::transaction::resource::commit::reply( state),
            local::external::transaction::resource::rollback::reply( state),

            // discover
            local::external::domain::discovery::reply( state)
         };
      }

      void unadvertise( state::Lookup::Resources resources)
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
         std::optional< configuration::model::gateway::outbound::Connection> lost( State& state, strong::file::descriptor::id descriptor)
         {
            Trace trace{ "gateway::group::outbound::handle::connection::lost"};
            log::line( verbose::log, "descriptor: ", descriptor);

            // unadvertise all 'orphanage' services and queues, if any.
            handle::unadvertise( state.lookup.remove( descriptor));

            // take care of aggregated replies, if any.
            state.coordinate.discovery.failed( descriptor);

            auto error_reply = []( auto& point){ error::reply::point( point);};

            // consume routs associated with the 'connection', and try to send 'error-replies'
            algorithm::for_each( state.route.message.consume( descriptor), error_reply);
            algorithm::for_each( state.route.service.message.consume( descriptor), error_reply);

            // connection might have been in 'disconnecting phase'
            algorithm::trim( state.disconnecting, algorithm::remove( state.disconnecting, descriptor));

            // remove the information about the 'connection'.
            return state.external.remove( state.directive, descriptor);
         }

         void disconnect( State& state, common::strong::file::descriptor::id descriptor)
         {
            Trace trace{ "gateway::group::outbound::handle::connection::disconnect"};
            log::line( verbose::log, "descriptor: ", descriptor);

            // unadvertise all 'orphanage' services and queues, if any.
            handle::unadvertise( state.lookup.remove( descriptor));

            // take care of aggregated replies, if any.
            state.coordinate.discovery.failed( descriptor);

            state.disconnecting.push_back( descriptor);
         }
         
      } // connection

      void connect( State& state, communication::tcp::Duplex&& device, configuration::model::gateway::outbound::Connection configuration)
      {
         Trace trace{ "gateway::group::outbound::handle::connect"};

         gateway::message::domain::connect::Request request;
         request.domain = common::domain::identity();
         request.correlation = strong::correlation::id::emplace( uuid::make());
         request.versions = range::to_vector( gateway::message::domain::protocol::versions);
         
         log::line( verbose::log, "request: ", request);

         state.route.message.emplace(
            communication::device::blocking::send( device, request), // TODO: non::blocking...
            process::handle(), 
            common::message::type( request), 
            device.connector().descriptor());

         state.external.add( 
            state.directive, 
            std::move( device),
            configuration);

      }

      std::vector< configuration::model::gateway::outbound::Connection> idle( State& state)
      {
         if( state.disconnecting.empty())
            return {};

         auto no_pending = [&state]( auto& descriptor)
         {
            return ! state.route.service.message.associated( descriptor) 
               && ! state.route.message.associated( descriptor);
         };

         std::vector< configuration::model::gateway::outbound::Connection> result;

         for( auto descriptor : algorithm::extract( state.disconnecting, algorithm::filter( state.disconnecting, no_pending)))
            if( auto configuration = handle::connection::lost( state, descriptor))
               result.push_back( std::move( configuration.value()));

         return result;
      }

      void shutdown( State& state)
      {
         Trace trace{ "gateway::group::outbound::handle::shutdown"};

         state.runlevel = state::Runlevel::shutdown;

         // unadvertise all resources
         handle::unadvertise( state.lookup.resources());

         // send metric for good measure 
         if( ! state.route.service.metric.metrics.empty())
         {
            ipc::flush::optional::send( ipc::manager::service(), state.route.service.metric);
            state.route.service.metric.metrics.clear();
         }

         for( auto descriptor : range::to_vector( state.external.descriptors))
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

         auto error_reply = []( auto& point){ error::reply::point( point);};
         algorithm::for_each( state.route.message.consume(), error_reply);
      }

   } // gateway::group::outbound::handle
} // casual