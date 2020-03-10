//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/outbound/route.h"
#include "gateway/outbound/error/reply.h"
#include "gateway/message.h"
#include "gateway/common.h"

#include "common/argument.h"
#include "common/exception/handle.h"
#include "common/exception/casual.h"
#include "common/communication/instance.h"
#include "common/communication/tcp.h"
#include "common/communication/select.h"
#include "common/execute.h"
#include "common/stream.h"

#include "common/message/service.h"
#include "common/message/handle.h"



namespace casual
{
   using namespace common;

   namespace gateway
   {
      namespace outbound
      {

         
         namespace local
         {
            namespace
            {
               using size_type = platform::size::type;

               namespace blocking
               {
                  template< typename D, typename M>
                  void send( D&& device, M&& message)
                  {
                     common::communication::ipc::blocking::send( std::forward< D>( device), std::forward< M>( message));
                  }


                  namespace optional
                  {
                     template< typename D, typename M>
                     bool send( D&& device, M&& message)
                     {
                        try
                        {
                           blocking::send( std::forward< D>( device), std::forward< M>( message));
                           return true;
                        }
                        catch( const common::exception::system::communication::Unavailable&)
                        {
                           log::line( log, "destination queue unavailable - device: ", device, ", message: ", message ," - action: discard");
                           return false;
                        }
                     }
                  } // optional
                  namespace transaction
                  {
                     template< typename M> 
                     void involved( M&& message)
                     {
                        if( message.trid)
                        {
                           blocking::send( 
                              common::communication::instance::outbound::transaction::manager::device(),
                              common::message::transaction::resource::external::involved::create( message));
                        }
                     }
                  } // transaction
               } // blocking

               struct Settings
               {
                  std::string address;
                  size_type order = 0;
               };

               struct State 
               {
                  
                  State( communication::tcp::inbound::Device&& inbound, size_type order)
                     : external{ std::move( inbound)}, order( order)
                  {
                     metric.metrics.reserve( platform::batch::gateway::metrics);
                  }

                  ~State()
                  {
                     log::line( verbose::log, "external branches: ", branch.branches());
                  }

                  State( State&&) = default;


                  struct 
                  {
                     communication::tcp::inbound::Device inbound;

                     template< typename M>
                     auto send( M&& message)
                     {
                        return communication::tcp::outbound::blocking::send( 
                           inbound.connector().socket(),
                           std::forward< M>( message));
                     }

                  } external;

                  communication::select::Directive directive;

                  struct 
                  {
                     route::service::Route route;
                  } service;

                  route::Route route;

                  common::message::event::service::Calls metric;
                  size_type order;

                  struct
                  {
                     struct Mapping 
                     {
                        Mapping( const common::transaction::ID& internal, const common::transaction::ID& external) 
                           : internal{ internal}, external{ external} {}

                        common::transaction::ID internal;
                        common::transaction::ID external;

                        friend std::ostream& operator << ( std::ostream& out, const Mapping& value) 
                        {
                           return out << "{ internal: " << value.internal << ", external: " << value.external << "}";
                        }
                     };
                     using transaction_branch_type = std::vector< Mapping>;

                     //! @return the (possible created) external branched trid.
                     const common::transaction::ID& external( const common::transaction::ID& trid)
                     {
                        auto find_internal = [&trid]( auto& m){ return m.internal == trid;};

                        auto found = algorithm::find_if( m_branches, find_internal);

                        if( found)
                           return found->external;

                        m_branches.emplace_back( trid, common::transaction::id::branch( trid));

                        return m_branches.back().external;
                     }

                     //! @return the internal branched trid, throws if not found
                     const common::transaction::ID& internal( const common::transaction::ID& trid) const
                     {
                        auto find_external = [&trid]( auto& m){ return m.external == trid;};

                        auto found = algorithm::find_if( m_branches, find_external);

                        if( found)
                           return found->internal;

                        throw exception::system::invalid::Argument{ string::compose( "failed to correlate the branchad external trid: ", trid)};
                     }                     

                     //! removes the correlation between external branched trid and the internal trid
                     void remove( const common::transaction::ID& external)
                     {
                        auto find_external = [&external]( auto& m){ return m.external == external;};

                        auto found = algorithm::find_if( m_branches, find_external);

                        if( found)
                        {
                           log::line( verbose::log, "remove branch mapping: ", *found);
                           m_branches.erase( std::begin( found));
                        }
                     }

                     const transaction_branch_type& branches() const { return m_branches;}

                  private: 
                     transaction_branch_type m_branches;
                  } branch;

                  //! correlations of all pending rediscoveries. 
                  std::vector< Uuid> rediscoveries;
               };

               namespace inbound
               {
                  template< typename Message>
                  auto message()
                  {
                     Trace trace{ "gateway::outbound::local::inbound::message"};

                     return common::message::dispatch::reply::pump< Message>(
                        common::message::handle::defaults( common::communication::ipc::inbound::device()),
                        common::communication::ipc::inbound::device());
                  }
               } // inbound


               auto connect( communication::tcp::Address address)
               {
                  Trace trace{ "gateway::outbound::local::connect"};

                  try
                  {
                     // we try to connect directly
                     return communication::tcp::connect( address);
                  }
                  catch( const exception::system::communication::Refused&)
                  {
                     // no-op
                  }

                  // no one is listening on the other side, yet... Let's start the
                  // retry dance...

                  communication::Socket socket;

                  auto signal_alarm = [&address, &socket]()
                  {
                     try
                     {
                        socket = communication::tcp::connect( address);
                        // we're connected, push message to our own ipc so we end
                        // our plocking message pump

                        communication::ipc::inbound::device().push( message::outbound::connect::Done{});
                     }
                     catch( const exception::system::communication::Refused&)
                     {
                        common::signal::timer::set( std::chrono::seconds{ 1});
                     }
                  };

                  auto restore = signal::callback::scoped::replace< code::signal::alarm>( std::move( signal_alarm));

                  // set the first timeout
                  common::signal::timer::set( std::chrono::seconds{ 1});

                  auto handler = communication::ipc::inbound::device().handler( 
                     common::message::handle::defaults( communication::ipc::inbound::device()), 
                     []( const message::outbound::connect::Done&) {} // no-op 
                     );
                  
                  common::message::dispatch::conditional::pump( 
                     handler,
                     communication::ipc::inbound::device(),
                     [&socket]()
                     {
                        return socket ? true : false;
                     });

                  return socket;
               }


               State connect( Settings&& settings)
               {
                  Trace trace{ "gateway::outbound::local::connect"};

                  // 'connect' to our local domain
                  common::communication::instance::connect();

                  // connect to the other domain, this will try until success or "shutdown"
                  communication::tcp::inbound::Device inbound{ connect( std::move( settings.address))};

                  // send connect request
                  {
                     Trace trace{ "gateway::outbound::local::connect send domain connect request"};

                     common::message::gateway::domain::connect::Request request;
                     request.domain = common::domain::identity();
                     request.versions = { common::message::gateway::domain::protocol::Version::version_1};
                     
                     log::line( verbose::log, "request: ", request);

                     communication::tcp::outbound::blocking::send( inbound.connector().socket(), request);
                  }

                  // the reply will be handled in the main message pump

                  return State{ std::move( inbound), settings.order};
               }

               namespace handle
               {
                  namespace advertise
                  {
                     namespace detail
                     {
                        template< typename C>
                        void services( State& state, const Uuid& execution, C&& complement)
                        {
                           common::message::service::concurrent::Advertise advertise;
                           advertise.execution = execution;
                           advertise.process = common::process::handle();
                           advertise.order = state.order;

                           complement( advertise);

                           blocking::send( 
                              common::communication::instance::outbound::service::manager::device(), 
                              advertise);
                        }

                        template< typename C>
                        void queues( State& state, const Uuid& execution, C&& complement)
                        {
                           common::message::queue::concurrent::Advertise advertise;
                           advertise.execution = execution;
                           advertise.process = common::process::handle();
                           advertise.order = state.order;
                           complement( advertise);

                           blocking::optional::send( 
                              common::communication::instance::outbound::queue::manager::optional::device(),
                              advertise);
                        }
                     } // detail

                     template< typename S = std::vector< common::message::service::concurrent::advertise::Service>>
                     void services( State& state, const Uuid& execution, S&& services)
                     {
                        detail::services( state, execution, [&services]( auto& message)
                        {
                           message.services.add = std::forward< S>( services);

                           // add one hop, since we now it has passed a domain boundary
                           for( auto& service : message.services.add) 
                              ++service.hops;
                        });
                     }


                     template< typename Q = std::vector< common::message::queue::concurrent::advertise::Queue>>
                     void queues( State& state, const Uuid& execution, Q&& queues)
                     {
                        detail::queues( state, execution, [&queues]( auto& message)
                        {
                           message.queues.add = std::forward< Q>( queues);
                        });
                     }

                     namespace reset
                     {
                        template< typename S = std::vector< common::message::service::concurrent::advertise::Service>>
                        void services( State& state, const Uuid& execution, S&& services)
                        {
                           detail::services( state, execution, [&services]( auto& message)
                           {
                              message.reset = true;
                              message.services.add = std::forward< S>( services);

                              // add one hop, since we now it has passed a domain boundary
                              for( auto& service : message.services.add) 
                                 ++service.hops;
                           });
                        }


                        template< typename Q = std::vector< common::message::queue::concurrent::advertise::Queue>>
                        void queues( State& state, const Uuid& execution, Q&& queues)
                        {
                           detail::queues( state, execution, [&queues]( auto& message)
                           {
                              message.reset = true;
                              message.queues.add = std::forward< Q>( queues);
                           });
                        }
                        
                     } // reset

                  } // advertise


                  namespace external
                  {
                     namespace origin
                     {
                        template< typename M>
                        void transaction( State& state, M& message)
                        {
                           if( message.transaction.trid)
                           {
                              message.transaction.trid = state.branch.internal( message.transaction.trid);

                              log::line( verbose::log, "internal trid: ", message.transaction.trid);
                           }
                        }

                     } // branch

                     namespace connect
                     {
                        auto reply( State& state)
                        {
                           return [&state]( const common::message::gateway::domain::connect::Reply& message)
                           {
                              Trace trace{ "gateway::outbound::local::handle::external::connect::reply"};
                              log::line( verbose::log, "message: ", message);
                
                              if( message.version != common::message::gateway::domain::protocol::Version::version_1)
                                 throw common::exception::system::invalid::Argument{ "invalid protocol"};

                              // connect to gateway
                              {
                                 message::outbound::Connect connect;
                                 connect.process = common::process::handle();
                                 connect.domain = message.domain;
                                 connect.version = message.version;
                                 const auto& socket =  state.external.inbound.connector().socket();
                                 connect.address.local = communication::tcp::socket::address::host( socket);
                                 connect.address.peer = communication::tcp::socket::address::peer( socket);

                                 log::line( verbose::log, "connect: ", connect);

                                 common::communication::ipc::blocking::send(
                                    common::communication::instance::outbound::gateway::manager::device(),
                                    connect);
                              }

                              // ask for configuration, reply will be handle by the main message pump
                              {                              
                                 message::outbound::configuration::Request request;
                                 request.process = common::process::handle();

                                 common::communication::ipc::blocking::send(
                                    common::communication::instance::outbound::gateway::manager::device(),
                                    request);
                              }
                           };
                        }
                     } // connect

                     namespace basic
                     {
                        namespace detail
                        {
                           // sets process if message has a process attribute
                           template< typename M>
                           auto process( M& message, traits::priority::tag< 1>) -> decltype( message.process = common::process::handle(), void())
                           { 
                              message.process = common::process::handle();
                           }

                           template< typename M>
                           void process( M& message, traits::priority::tag< 0>) {}
                        } // detail

                        template< typename Message>
                        auto reply( State& state)
                        {
                           return [&state]( Message& message)
                           {
                              Trace trace{ "gateway::outbound::local::handle::external::basic_reply"};
                              log::line( verbose::log, "message: ", message);

                              try
                              {
                                 auto destination = state.route.get( message.correlation);
                                 detail::process( message, traits::priority::tag< 1>{});

                                 blocking::optional::send( destination.destination.ipc, message);
                              }
                              catch( const common::exception::system::invalid::Argument&)
                              {
                                 log::line( log::category::error, "failed to correlate [", message.correlation, "] reply with a destination - action: ignore");
                              }
                           };
                        }
                     } // basic



                     namespace service
                     {
                        namespace call
                        {
                           auto reply( State& state)
                           {
                              return [&state]( common::message::service::call::Reply& message)
                              {
                                 Trace trace{ "gateway::outbound::local::handle::external::service::call::reply"};
                                 log::line( verbose::log, "message: ", message);

                                 try
                                 {
                                    auto destination = state.service.route.get( message.correlation);

                                    // get the original "un-branched" trid
                                    external::origin::transaction( state, message);

                                    auto now = platform::time::clock::type::now();

                                    state.metric.metrics.push_back( [&]()
                                    {
                                       common::message::event::service::Metric metric;
                                       metric.process = common::process::handle();
                                       metric.execution = message.execution;
                                       metric.service = std::move( destination.service);
                                       metric.parent = std::move( destination.parent);
                                       
                                       metric.trid = message.transaction.trid;
                                       metric.start = destination.start;
                                       metric.end = now;

                                       metric.code = message.code.result;

                                       return metric;
                                    }());

                                    blocking::optional::send( destination.destination.ipc, message);
                                 }
                                 catch( const common::exception::system::invalid::Argument&)
                                 {
                                    log::line( log::category::error, "failed to correlate [", message.correlation, "] reply with a destination - action: ignore");
                                    log::line( verbose::log, "state.service.route: ", state.service.route);
                                 }

                                 // send service metrics if we don't have any more in-flight call request (this one
                                 // was the last, or only) OR we've accumulated enough metrics for a batch update
                                 if( state.service.route.empty() || state.metric.metrics.size() >= platform::batch::gateway::metrics)
                                 {
                                    blocking::send( common::communication::instance::outbound::service::manager::device(), state.metric);
                                    state.metric.metrics.clear();
                                 }
                              };
                           }

                        } // call

                     } // service

                     namespace queue
                     {
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
                           auto reply( State& state)
                           {
                              return [&state]( common::message::gateway::domain::discover::Reply& message)
                              {
                                 Trace trace{ "gateway::outbound::handle::domain::discover::Reply"};
                                 log::line( verbose::log, "message: ", message);

                                 auto destination = state.route.get( message.correlation);

                                 if( auto found = algorithm::find( state.rediscoveries, message.correlation))
                                 {
                                    Trace trace{ "gateway::outbound::handle::domain::discover::Reply rediscover"};

                                    state.rediscoveries.erase( std::begin( found));

                                    handle::advertise::reset::services( state, message.execution, message.services);
                                    handle::advertise::reset::queues( state, message.execution, message.queues);

                                    auto reply = message::outbound::rediscover::Reply{ common::process::handle()};
                                    reply.correlation = message.correlation;
                                    reply.execution = message.execution;
                                    blocking::optional::send( destination.destination.ipc, reply);

                                    return;
                                 }

                                 // advertise
                                 {
                                    Trace trace{ "gateway::outbound::handle::domain::discover::Reply advertise"};

                                    if( ! message.services.empty())
                                       handle::advertise::services( state, message.execution, message.services);

                                    if( ! message.queues.empty())
                                       handle::advertise::queues( state, message.execution, message.queues);
                                 }

                                 // route only if we're not the caller (we use this in the configuration step)
                                 if( destination.destination != process::handle())
                                 {
                                    Trace trace{ "gateway::outbound::handle::domain::discover::Reply forward reply"};

                                    message.process = common::process::handle();

                                    blocking::optional::send( destination.destination.ipc, message);
                                 }
                              };
                           }
                        } // discover
                     } // domain



                     auto handler( State& state)
                     {
                        return state.external.inbound.handler(
                           // the reply from the other side, will only be invoked once.
                           connect::reply( state),
                           
                           // service
                           service::call::reply( state),

                           // queue
                           queue::enqueue::reply( state),
                           queue::dequeue::reply( state),

                           // transaction
                           transaction::resource::prepare::reply( state),
                           transaction::resource::commit::reply( state),
                           transaction::resource::rollback::reply( state),

                           // discover
                           domain::discover::reply( state)
                        );
                     }

                     namespace create
                     {
                        auto dispatch( State& state)
                        {
                           const auto descriptor = state.external.inbound.connector().descriptor();
                           state.directive.read.add( descriptor);

                           return communication::select::dispatch::create::reader(
                              descriptor,
                              [&device = state.external.inbound, handler = external::handler( state)]( auto active) mutable
                              {
                                 handler( device.next( device.policy_non_blocking()));
                              }
                           );

                        }
                     } // create
                  } // external

                  namespace internal
                  {
                     namespace branch
                     {
                        template< typename M>
                        void transaction( State& state, M& message)
                        {
                           message.trid = state.branch.external( message.trid);

                           log::line( verbose::log, "external trid: ", message.trid);
                        }
                     } // branch

                     namespace basic
                     {
                        auto send = []( State& state, auto& message)
                        {
                           Trace trace{ "gateway::outbound::local::handle::internal::basic::send"};

                           // add route so we know where to send it when the reply arrives
                           state.route.add( message);

                           log::line( verbose::log, "message: ", message);

                           state.external.send( message);
                        };
                        
                        template< typename Message>
                        auto request( State& state)
                        {
                           return [&state]( Message& message)
                           {
                              basic::send( state, message);
                           };
                        }
                     } // basic



                     namespace service
                     {
                        namespace call
                        {
                           auto request( State& state)
                           {
                              return [&state]( common::message::service::call::callee::Request& message)
                              {
                                 Trace trace{ "gateway::outbound::local::handle::internal::service::call::Request"};
                                 log::line( verbose::log, "message: ", message);

                                 auto now = platform::time::clock::type::now();

                                 if( ! message.flags.exist( common::message::service::call::request::Flag::no_reply))
                                 {
                                    // call within a transaction can only happend here (with a reply)
                                    if( message.trid)
                                    {
                                       // branch 
                                       branch::transaction( state, message);

                                       // notify TM that this "resource" is involved in the branched transaction
                                       blocking::transaction::involved( message);
                                    }
                           
                                    state.service.route.emplace(
                                       message.correlation,
                                       message.process,
                                       message.service.name,
                                       message.parent,
                                       now
                                    );
                                 }
                                 state.external.send( message);
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
                                    Trace trace{ "gateway::outbound::local::handle::internal::service::conversation::connect::Request"};

                                    log::line( verbose::log, "message: ", message);

                                    if( message.trid)
                                    {
                                       // branch 
                                       branch::transaction( state, message);

                                       // notify TM that this "resource" is involved in the branched transaction
                                       blocking::transaction::involved( message);
                                    }

                                    state.route.add( message);
                                    state.external.send( message);
                                 };
                              }

                           } // connect

                        } // conversation
                     } // service

                     namespace domain
                     {
                        namespace discover
                        {
                           auto request = basic::request< common::message::gateway::domain::discover::Request>;
                        } // discover

                        namespace rediscover
                        {
                           auto request( State& state)
                           {
                              return [&state]( message::outbound::rediscover::Request& message)
                              {
                                 Trace trace{ "gateway::outbound::local::handle::internal::domain::rediscover::request"};
                                 log::line( verbose::log, "message: ", message);

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
                              };
                           }
                        } // rediscover
                     } // domain

                     namespace transaction
                     {
                        namespace resource
                        {    
                           namespace basic
                           {
                              template< typename Message>
                              auto request( State& state)
                              {
                                 return [&state]( Message& message)
                                 {
                                    Trace trace{ "gateway::outbound::local::handle::internal::transaction::resource::basic::request"};

                                    // remove the trid correlation, since we're in the commit/rollback dance
                                    state.branch.remove( message.trid);

                                    internal::basic::send( state, message);
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
                           } // commit
                        } // resource
                     } // transaction

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

                                 // we don't bother with branching on casual-queue, since it does not mather.

                                 // potentially notify TM that this "resource" is involved in the transaction
                                 blocking::transaction::involved( message);
                                 
                                 internal::basic::send( state, message);
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

                     namespace configuration
                     {
                        auto reply( State& state)
                        {
                           return [&state]( const message::outbound::configuration::Reply& message)
                           {
                              Trace trace{ "gateway::outbound::local::handle::internal::configuration::reply"};
                              log::line( verbose::log, "message: ", message);

                              if( ! message.services.empty() || ! message.queues.empty())
                              {
                                 // We make sure we get the reply (hence not forwarding to some other process)
                                 common::message::gateway::domain::discover::Request request{ common::process::handle()};

                                 // make sure we have a correlation id.
                                 request.correlation = common::uuid::make();

                                 request.domain = common::domain::identity();
                                 request.services = message.services;
                                 request.queues = message.queues;

                                 // Use regular handler to manage the "routing"
                                 handle::internal::domain::discover::request( state)( request);
                              }
                           };

                        }
                     } // configuration

                     auto handler( State& state)
                     {
                        auto& device = communication::ipc::inbound::device();
                        return device.handler(
                           
                           common::message::handle::defaults( device),

                           configuration::reply( state),

                           // service
                           service::call::request( state),
                           service::conversation::connect::request( state),
                           
                           // queue
                           queue::dequeue::request( state),
                           queue::enqueue::request( state),

                           // transaction
                           transaction::resource::prepare::request( state),
                           transaction::resource::commit::request( state),
                           transaction::resource::rollback::request( state),

                           // discover
                           domain::discover::request( state),
                           domain::rediscover::request( state)
                        );
                     }

                     namespace create
                     {
                        using handler_type = typename communication::ipc::inbound::Device::handler_type;
                        struct Dispatch
                        {
                           Dispatch( State& state) : m_handler( internal::handler( state)) 
                           {
                              state.directive.read.add( communication::ipc::inbound::handle().socket().descriptor());
                           }
                           auto descriptor() const { return communication::ipc::inbound::handle().socket().descriptor();}

                           void operator () ( strong::file::descriptor::id descriptor)
                           {
                              consume();
                           }

                           bool consume()
                           {
                              auto& device = common::communication::ipc::inbound::device();  
                              return m_handler( device.next( device.policy_non_blocking()));
                           } 
                           handler_type m_handler;
                        };

                        auto dispatch( State& state)
                        {
                           return Dispatch( state);
                        }
                     } // create

                  } // internal
               } // handle

        
               void start( State&& state)
               {
                  Trace trace{ "gateway::outbound::local::start"};

                  log::line( log::category::information, "outbound connected: ", 
                     communication::tcp::socket::address::peer( state.external.inbound.connector().socket()));
                  
                  // we make sure to send error replies for any pending in-flight requests.
                  auto send_error_replies = execute::scope( [&state](){
                     error::reply( state.route);
                     error::reply( state.service.route);
                  });

                  {
                     Trace trace{ "gateway::outbound::local::start dispatch pump"};

                     // start the message dispatch
                     communication::select::dispatch::pump( 
                        state.directive, 
                        handle::internal::create::dispatch( state),
                        handle::external::create::dispatch( state)
                     );
                  }

               }

               void main( int argc, char* argv[])
               {
                  Settings settings;

                  argument::Parse{ "tcp outbound",
                     argument::Option( std::tie( settings.address), { "-a", "--address"}, "address to the remote domain [(ip|domain):]port"),
                     argument::Option( std::tie( settings.order), { "-o", "--order"}, "order of the outbound connector"),
                  }( argc, argv);

                  start( connect( std::move( settings)));
    
               }
            } // <unnamed>
         } // local


      } // outbound
   } // gateway
} // casual

int main( int argc, char* argv[])
{
   return casual::common::exception::guard( [=]()
   {
      casual::gateway::outbound::local::main( argc, argv);
   });
} // main

