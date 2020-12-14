//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/reverse/outbound/handle.h"

#include "gateway/message.h"
#include "gateway/common.h"

#include "common/communication/instance.h"
#include "common/message/handle.h"

namespace casual
{
   using namespace common;

   namespace gateway::reverse::outbound::handle
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
                  auto& transaction() { return communication::instance::outbound::transaction::manager::device();}
                  namespace optional
                  {
                     auto& queue() { return communication::instance::outbound::queue::manager::optional::device();}
                  } // optional

               } // manager

               auto& inbound() { return communication::ipc::inbound::device();}
            } // ipc

            namespace tcp
            {
               template< typename M>
               void send( State& state, strong::file::descriptor::id descriptor, M&& message)
               {
                  try
                  {
                     if( auto connection = state.external.connection( descriptor))
                     {
                        communication::device::blocking::send( 
                           connection->device,
                           std::forward< M>( message));
                     }
                  }
                  catch( ...)
                  {
                     exception::sink::error();
                  }
               }

               
            } // tcp

            namespace internal
            {
               namespace transaction
               {
                  template< typename M>
                  void branch( State& state, M& message)
                  {
                        message.trid = state.lookup.external( message.trid);
                        log::line( verbose::log, "external trid: ", message.trid);
                  }

                  template< typename M> 
                  void involved( M&& message)
                  {
                     if( message.trid)
                     {
                        communication::device::blocking::send( 
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
                              Trace trace{ "gateway::reverse::outbound::local::handle::internal::transaction::resource::basic::request"};
                              log::line( verbose::log, "message: ", message);

                              tcp::send( state, state.lookup.connection( message.trid), message);
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
                     auto request( State& state)
                     {
                        return [&state]( common::message::service::call::callee::Request& message)
                        {
                           Trace trace{ "gateway::reverse::outbound::handle::local::internal::service::call::request"};
                           log::line( verbose::log, "message: ", message);

                           auto now = platform::time::clock::type::now();

                           auto [ lookup, involved] = state.lookup.service( message.service.name, message.trid);

                           message.trid = lookup.trid;
                           
                           // notify TM that this "resource" is involved in the branched transaction
                           if( involved)
                              transaction::involved( message);

                           if( ! message.flags.exist( common::message::service::call::request::Flag::no_reply))
                           {                              
                              state.route.service.message.emplace(
                                 message.correlation,
                                 message.process,
                                 message.service.name,
                                 message.parent,
                                 now,
                                 lookup.connection
                              );
                           }
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
                              Trace trace{ "gateway::reverse::outbound::handle::local::internal::service::conversation::connect::request"};
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
                  namespace discover
                  {
                     namespace detail
                     {
                        auto coordinate( const strong::ipc::id& destination, const Uuid& correlation)
                        {
                           return [destination, correlation]( auto replies, auto failed)
                           {
                              Trace trace{ "gateway::reverse::outbound::handle::local::internal::domain::discover::detail::coordinate"};

                              common::message::gateway::domain::discover::Reply message{ process::handle()};
                              message.correlation = correlation;

                              algorithm::for_each( replies, [&message]( auto& reply)
                              {
                                 algorithm::append( reply.services, message.services);
                                 algorithm::append( reply.queues, message.queues);
                              });

                              communication::device::blocking::optional::send( destination, message);
                           };
                        }
                        
                     } // detail

                     auto request( State& state)
                     {
                        return [&state]( common::message::gateway::domain::discover::Request& message)
                        {
                           Trace trace{ "gateway::reverse::outbound::handle::local::internal::domain::discover::request"};
                           log::line( verbose::log, "message: ", message);

                           // make sure we get unique correlations for the coordination
                           auto correlation = std::exchange( message.correlation, {});

                           auto send_requests = []( State& state, auto& message)
                           {
                              std::vector< state::coordinate::discovery::Message::Pending> result;

                              algorithm::for_each( state.external.connections, [&state, &result, &message]( auto& connection)
                              {
                                 if( auto correlation = communication::device::blocking::optional::send( connection.device, message))
                                 {
                                    auto descriptor = connection.device.connector().descriptor();
                                    state.route.message.emplace( correlation, message.process, message.type(), descriptor);
                                    result.emplace_back( correlation, descriptor);
                                 }
                              });

                              log::line( verbose::log, "pending: ", result);
                              
                              return result;
                           };

                           state.coordinate.discovery( 
                              send_requests( state, message),
                              detail::coordinate( message.process.ipc, correlation)
                           );

                        };
                     }
                  } // discover

                  namespace rediscover
                  {
                     auto request( State& state)
                     {
                        return []( message::outbound::rediscover::Request& message)
                        {
                           Trace trace{ "gateway::reverse::outbound::handle::local::internal::domain::rediscover::request"};
                           log::line( verbose::log, "message: ", message);

                           /*

                           if( message.services.empty() && message.queues.empty())
                           {
                              // noting to discover, we reset all our services and queues,
                              handle::advertise::reset::services( state, message.execution, {});
                              handle::advertise::reset::queues( state, message.execution, {});

                              // and send the reply directly                                    
                              blocking::optional::send( message.process.ipc, common::message::reverse::type( message, common::process::handle()));

                              return;
                           }

                           // We need to (re)discover the other side...
                           // we use the general _route_ to keep track of destination when we get the reply
                           common::message::gateway::domain::discover::Request request{ message.process};
                           request.correlation = message.correlation;
                           request.domain = common::domain::identity();
                           request.services = std::move( message.services);
                           request.queues = std::move( message.queues);

                           basic::send( state, request);
                           
                           state.rediscoveries.push_back( message.correlation);
                           */
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
                           Trace trace{ "gateway::reverse::outbound::local::handle::internal::queue::basic::request"};
                           log::line( verbose::log, "message: ", message);

                           auto [ lookup, involved] = state.lookup.queue( message.name, message.trid);

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
                     auto request = basic::request< common::message::queue::enqueue::Request>;
                  } // enqueue

                  namespace dequeue
                  {
                     auto request = basic::request< common::message::queue::dequeue::Request>;
                  } // dequeue
               } // queue
               
            } // internal

            namespace external
            {
               namespace connect
               {
                  auto reply( State& state)
                  {
                     return [&state]( const common::message::gateway::domain::connect::Reply& message)
                     {
                        Trace trace{ "gateway::reverse::outbound::handle::local::external::connect::reply"};
                        log::line( verbose::log, "message: ", message);

                        auto destination = state.route.message.consume( message.correlation);

                        if( ! destination)
                        {
                           log::line( log::category::error, code::casual::internal_correlation, " failed to correlate [", message.correlation, "] reply with a destination - action: ignore");
                           return;
                        }

                        if( message.version != common::message::gateway::domain::protocol::Version::version_1)
                           code::raise::error( code::casual::invalid_version, "invalid protocol version: ", message.version);

                        if( auto information = algorithm::find( state.external.information, destination.connection))
                        {
                           information->domain = message.domain;
                           
                           // should we do discovery
                           if( information->configuration)
                           {
                              // We make sure we get the reply (hence not forwarding to some other process)
                              common::message::gateway::domain::discover::Request request{ common::process::handle()};

                              request.correlation = message.correlation;

                              request.domain = common::domain::identity();
                              request.services = information->configuration.services;
                              request.queues = information->configuration.queues;

                              state.route.message.add( request, destination.connection);
                              tcp::send( state, destination.connection, request);
                           }
                        }
                     };
                  }
               } // connect

               namespace service
               {
                  namespace call
                  {
                     auto reply( State& state)
                     {
                        return [&state]( common::message::service::call::Reply& message)
                        {
                           Trace trace{ "gateway::reverse::outbound::handle::local::external::service::call::reply"};
                           log::line( verbose::log, "message: ", message);


                           auto no_entry_unadvertise = []( State& state, auto& reply, auto& destination)
                           {
                              if( reply.code.result != decltype( reply.code.result)::no_entry)
                                 return; // no-op

                              auto services = state.lookup.remove( destination.connection, { destination.service}, {}).services;

                              if( services.empty())
                                 return; 
                              
                              common::message::service::Advertise message{ common::process::handle()};
                              message.services.remove = std::move( services);
                              communication::device::blocking::optional::send( ipc::manager::service(), message);
                           };

                           if( auto destination = state.route.service.message.consume( message.correlation))
                           {
                              // we unadvertise the service if we get no_entry, and we got 
                              // no connections left for the service
                              no_entry_unadvertise( state, message, destination);

                              // get the internal "un-branched" trid
                              message.transaction.trid = state.lookup.internal( message.transaction.trid);

                              auto now = platform::time::clock::type::now();

                              state.route.service.metric.metrics.push_back( [&]()
                              {
                                 common::message::event::service::Metric metric;
                                 metric.process = common::process::handle();
                                 metric.execution = message.execution;
                                 metric.service = std::move( destination.service);
                                 metric.parent = std::move( destination.parent);
                                 metric.type = decltype( metric.type)::concurrent;
                                 
                                 metric.trid = message.transaction.trid;
                                 metric.start = destination.start;
                                 metric.end = now;

                                 metric.code = message.code.result;

                                 return metric;
                              }());

                              communication::device::blocking::optional::send( destination.process.ipc, message);
                           }
                           else
                           {
                              log::line( log::category::error, code::casual::internal_correlation, " failed to correlate [", message.correlation, "] reply with a destination - action: ignore");
                              log::line( verbose::log, "state.route.service.message: ", state.route.service.message);
                           }

                           // send service metrics if we don't have any more in-flight call request (this one
                           // was the last, or only) OR we've accumulated enough metrics for a batch update
                           if( state.route.service.message.empty() || state.route.service.metric.metrics.size() >= platform::batch::gateway::metrics)
                           {
                              communication::device::blocking::optional::send( ipc::manager::service(), state.route.service.metric);
                              state.route.service.metric.metrics.clear();
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
                           Trace trace{ "gateway::reverse::outbound::handle::local::external::queue::basic::reply"};
                           log::line( verbose::log, "message: ", message);

                           if( auto destination = state.route.message.consume( message.correlation))
                           {
                              communication::device::blocking::optional::send( destination.process.ipc, message);
                           }
                           else
                              log::line( log::category::error, code::casual::internal_correlation, " failed to correlate [", message.correlation, "] reply with a destination - action: ignore");
                        };
                     }
                  } // basic

                  namespace enqueue
                  {
                     auto reply = basic::reply< common::message::queue::enqueue::Reply>;
                  } // enqueue

                  namespace dequeue
                  {
                     auto reply = basic::reply< common::message::queue::dequeue::Reply>;
                  } // dequeue
               } // queue

               namespace transaction
               {
                  namespace resource
                  {
                     namespace basic
                     {
                        template< typename Message>
                        auto reply( State& state)
                        {
                           return [&state]( Message& message)
                           {
                              Trace trace{ "gateway::reverse::outbound::handle::local::external::basic::reply"};
                              log::line( verbose::log, "message: ", message);

                              if( auto destination = state.route.message.consume( message.correlation))
                              {
                                 message.process = process::handle();
                                 communication::device::blocking::optional::send( destination.process.ipc, message);
                              }
                              else
                                 log::line( log::category::error, code::casual::internal_correlation, " failed to correlate [", message.correlation, "] reply with a destination - action: ignore");
                           };
                        }

                     } // basic

                     namespace prepare
                     {
                        auto reply = basic::reply< common::message::transaction::resource::prepare::Reply>;
                     } // prepare
                     namespace commit
                     {
                        auto reply = basic::reply< common::message::transaction::resource::commit::Reply>;
                     } // commit
                     namespace rollback
                     {
                        auto reply = basic::reply< common::message::transaction::resource::rollback::Reply>;
                     } // commit
                  } // resource
               } // transaction

               namespace domain
               {
                  namespace discover
                  {
                     namespace detail
                     {
                        void adevertise( State& state, strong::file::descriptor::id connection, common::message::gateway::domain::discover::Reply& message)
                        {
                           Trace trace{ "gateway::reverse::outbound::handle::local::external::domain::discover::detail::adevertise"};

                           auto get_name = []( auto& resource){ return resource.name;};

                           auto advertise = state.lookup.add( 
                              connection, 
                              algorithm::transform( message.services, get_name), 
                              algorithm::transform( message.queues, get_name));

                           auto equal_name = []( auto& lhs, auto& rhs){ return lhs.name == rhs;};

                           if( auto services = std::get< 0>( algorithm::intersection( message.services, advertise.services, equal_name)))
                           {
                              common::message::service::concurrent::Advertise request;
                              request.process = common::process::handle();
                              
                              request.order = state.order;
                              algorithm::copy( services, request.services.add);

                              communication::device::blocking::send( ipc::manager::service(), request);
                           }

                           if( auto queues = std::get< 0>( algorithm::intersection( message.queues, advertise.queues, equal_name)))
                           {
                              common::message::queue::concurrent::Advertise request;
                              request.process = common::process::handle();
                              // TOOD: request.order = state.order;
                              algorithm::copy( queues, request.queues.add);

                              communication::device::blocking::send( ipc::manager::optional::queue(), request);
                           }
                        }                        
                     } // detail


                     auto reply( State& state)
                     {
                        return [&state]( common::message::gateway::domain::discover::Reply& message)
                        {
                           Trace trace{ "gateway::reverse::outbound::handle::local::external::domain::discover::reply"};
                           log::line( verbose::log, "message: ", message);

                           auto destination = state.route.message.consume( message.correlation);

                           if( ! destination)
                           {
                              log::line( log::category::error, code::casual::internal_correlation, " failed to correlate [", message.correlation, "] reply with a destination - action: ignore");
                              return;
                           }

                           // advertise services and/or queues, if any.
                           detail::adevertise( state, destination.connection, message);

                           // coordinate only if we're not the caller (we use this in the configuration step)
                           if( destination.process != process::handle())
                           {
                              Trace trace{ "gateway::reverse::outbound::handle::local::external::domain::discover::reply forward reply"};

                              message.process = common::process::handle();
                              state.coordinate.discovery( std::move( message));
                           }
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
            common::message::handle::defaults( local::ipc::inbound()),

            //configuration::reply( state),

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
            local::internal::domain::discover::request( state),
            local::internal::domain::rediscover::request( state),
         };
      }

      external_handler external( State& state)
      {
         return {
            local::external::connect::reply( state),
            
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
            local::external::domain::discover::reply( state)
         };
      }

   } // gateway::reverse::outbound::handle
} // casual