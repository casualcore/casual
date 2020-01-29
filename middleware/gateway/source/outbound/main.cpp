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

                  // connect to the other domain, this will try unitl succes or "shutdown"
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
                  namespace state
                  {
                     struct Base
                     {
                        Base( State& state) : state( state) {}
                        State& state;
                     };
                  } // state


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

                     template< typename M>
                     struct basic_reply : state::Base
                     {
                        using message_type = M;
                        using state::Base::Base;

                        void operator() ( message_type& message) const
                        {
                           log::line( verbose::log, "message: ", message);

                           try
                           {
                              auto destination = state.route.get( message.correlation);
                              set_process( message);

                              blocking::optional::send( destination.destination.ipc, message);
                           }
                           catch( const common::exception::system::invalid::Argument&)
                           {
                              log::line( log::category::error, "failed to correlate [", message.correlation, "] reply with a destination - action: ignore");
                           }
                        }

                     private:
                        // todo: why can't we set source process for queue-messages? 
                        void set_process( common::message::queue::enqueue::Reply& message) const { }
                        void set_process( common::message::queue::dequeue::Reply& message) const { }

                        template< typename T>
                        void set_process( T& message) const { message.process = common::process::handle();}
                     };

                     namespace service
                     {
                        namespace call
                        {
                           struct Reply : state::Base
                           {
                              using state::Base::Base;

                              void operator() ( common::message::service::call::Reply& message)
                              {
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
                              }
                           };

                        } // call

                     } // service

                     namespace queue
                     {
                        namespace enqueue
                        {
                           using Reply = basic_reply< common::message::queue::enqueue::Reply>;
                        } // enqueue

                        namespace dequeue
                        {
                           using Reply = basic_reply< common::message::queue::dequeue::Reply>;
                        } // dequeue
                     } // queue

                     namespace transaction
                     {
                        namespace resource
                        {
                           namespace prepare
                           {
                              using Reply = basic_reply< common::message::transaction::resource::prepare::Reply>;
                           } // prepare
                           namespace commit
                           {
                              using Reply = basic_reply< common::message::transaction::resource::commit::Reply>;
                           } // commit
                           namespace rollback
                           {
                              using Reply = basic_reply< common::message::transaction::resource::rollback::Reply>;
                           } // commit
                        } // resource
                     } // transaction

                     namespace domain
                     {
                        namespace discover
                        {
                           struct Reply : state::Base 
                           {
                              using state::Base::Base;

                              void operator () ( common::message::gateway::domain::discover::Reply& message) const
                              {
                                 Trace trace{ "gateway::outbound::handle::domain::discover::Reply"};

                                 log::line( verbose::log, "message: ", message);

                                 auto destination = state.route.get( message.correlation);

                                 // advertise
                                 {
                                    Trace trace{ "gateway::outbound::handle::domain::discover::Reply advertise"};

                                    if( ! message.services.empty())
                                    {
                                       common::message::service::concurrent::Advertise advertise;
                                       advertise.execution = message.execution;
                                       advertise.process = common::process::handle();
                                       advertise.order = state.order;
                                       advertise.services = message.services;

                                       // add one hop, since we now it has passed a domain boundary
                                       for( auto& service : advertise.services) { ++service.hops;}


                                       blocking::send( 
                                          common::communication::instance::outbound::service::manager::device(), 
                                          advertise);
                                    }

                                    if( ! message.queues.empty())
                                    {
                                       common::message::queue::concurrent::Advertise advertise;
                                       advertise.execution = message.execution;
                                       advertise.process = common::process::handle();
                                       advertise.order = state.order;
                                       advertise.queues = message.queues;

                                       blocking::optional::send( 
                                          common::communication::instance::outbound::queue::manager::optional::device(),
                                          advertise);
                                    }
                                 }

                                 // route only if we're not the caller (we use this in the configuration step)
                                 if( destination.destination != process::handle())
                                 {
                                    Trace trace{ "gateway::outbound::handle::domain::discover::Reply forward reply"};

                                    message.process = common::process::handle();

                                    blocking::optional::send( destination.destination.ipc, message);
                                 }
                              }

                           };
                        } // discover
                     } // domain



                     auto handler( State& state)
                     {
                        return state.external.inbound.handler(
                           // the reply from the other side, will only be invoked once.
                           connect::reply( state),
                           
                           // service
                           service::call::Reply{ state},

                           // queue
                           queue::enqueue::Reply{ state},
                           queue::dequeue::Reply{ state},

                           // transaction
                           transaction::resource::prepare::Reply{ state},
                           transaction::resource::commit::Reply{ state},
                           transaction::resource::rollback::Reply{ state},

                           // discover
                           domain::discover::Reply{ state}
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
                     

                     template< typename M>
                     struct basic_request : state::Base
                     {
                        using state::Base::Base;
                        using message_type = M;

                        void operator() ( message_type& message)
                        {
                           Trace trace{ "gateway::outbound::local::handle::internal::basic_request"};

                           // add route so we know where to send it when the reply arrives
                           state.route.add( message);

                           log::line( verbose::log, "message: ", message);

                           state.external.send( message);
                        }
                     };

                     namespace service
                     {
                        namespace call
                        {
                           struct Request : state::Base
                           {
                              using state::Base::Base;

                              void operator() ( common::message::service::call::callee::Request& message)
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
                              }
                           };

                        } // call

                        namespace conversation
                        {
                           namespace connect
                           {
                              struct Request : state::Base
                              {
                                 using message_type = common::message::conversation::connect::callee::Request;
                                 using state::Base::Base;

                                 void operator() ( message_type& message)
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
                                 }
                              };

                           } // connect

                        } // conversation
                     } // service

                     namespace domain
                     {
                        namespace discover
                        {
                           using Request = basic_request< common::message::gateway::domain::discover::Request>;
                        } // discover
                     } // domain
                     namespace transaction
                     {
                        namespace resource
                        {                           
                           template< typename M>
                           struct basic_request : internal::basic_request< M>
                           {
                              using internal::basic_request< M>::basic_request;
                              using message_type = M;

                              void operator() ( message_type& message)
                              {
                                 Trace trace{ "gateway::outbound::local::handle::internal::transaction::resource::basic_request"};

                                 // remove the trid correlation, since we're in the commit/rollback dance
                                 this->state.branch.remove( message.trid);

                                 internal::basic_request< M>::operator() ( message);
                              }
                           };

                           namespace prepare
                           {
                              using Request = basic_request< common::message::transaction::resource::prepare::Request>;
                           } // prepare
                           namespace commit
                           {
                              using Request = basic_request< common::message::transaction::resource::commit::Request>;
                           } // commit
                           namespace rollback
                           {
                              using Request = basic_request< common::message::transaction::resource::rollback::Request>;
                           } // commit
                        } // resource
                     } // transaction

                     namespace queue
                     {
                        template< typename M>
                        struct basic_request : internal::basic_request< M>
                        {
                           using message_type = M;
                           using base_type = internal::basic_request< M>;

                           using base_type::base_type;

                           void operator() ( message_type& message)
                           {
                              Trace trace{ "gateway::outbound::local::handle::internal::queue::basic_request"};
                              log::line( verbose::log, "message: ", message);

                              // we don't bother with branching on casual-queue, since it does not bother.

                              // potentially notify TM that this "resource" is involved in the transaction
                              blocking::transaction::involved( message);
                              
                              base_type::operator()( message);
                           }
                        };

                        namespace enqueue
                        {
                           using Request = basic_request< common::message::queue::enqueue::Request>;
                        } // enqueue

                        namespace dequeue
                        {
                           using Request = basic_request< common::message::queue::dequeue::Request>;
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
                                 common::message::gateway::domain::discover::Request request;

                                 // make sure we have a correlation id.
                                 request.correlation = common::uuid::make();

                                 // We make sure we get the reply (hence not forwarding to some other process)
                                 request.process = common::process::handle();
                                 request.domain = common::domain::identity();
                                 request.services = message.services;
                                 request.queues = message.queues;

                                 // Use regular handler to manage the "routing"
                                 handle::internal::domain::discover::Request{ state}( request);
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
                           service::call::Request{ state},
                           service::conversation::connect::Request{ state},
                           
                           // queue
                           queue::dequeue::Request{ state},
                           queue::enqueue::Request{ state},

                           // transaction
                           transaction::resource::prepare::Request{ state},
                           transaction::resource::commit::Request{ state},
                           transaction::resource::rollback::Request{ state},

                           // discover
                           domain::discover::Request{ state}
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

