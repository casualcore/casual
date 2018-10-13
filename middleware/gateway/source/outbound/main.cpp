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
                     metric.process = common::process::handle();
                     metric.services.reserve( common::platform::batch::gateway::metrics);
                  }

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

                  common::message::service::concurrent::Metric metric;
                  size_type order;
               };

               namespace inbound
               {
                  template< typename Message>
                  auto message()
                  {
                     Trace trace{ "gateway::outbound::local::inbound::message"};

                     Message message;
                     
                     {
                        auto handler = common::communication::ipc::inbound::device().handler(
                           common::message::handle::Shutdown{},
                           // we don't handle ping until we're up'n running common::message::handle::ping(),
                           common::message::handle::assign( message));

                        while( ! message.correlation)
                        {
                           auto& device = common::communication::ipc::inbound::device();
                           handler( device.next( handler.types(), device.policy_blocking()));
                        }
                     }
                     return message;
                  }
               } // inbound

               State connect( Settings&& settings)
               {
                  Trace trace{ "gateway::outbound::local::connect"};

                  // 'connect' to our local domain
                  common::communication::instance::connect();

                  auto connect_thread = []( std::string&& address)
                  {
                     try
                     {
                        Trace trace{ "gateway::outbound::local::connect connect_thread"};

                        // We're only interested in sig-user
                        common::signal::thread::scope::Mask block{ common::signal::set::filled( common::signal::Type::user)};

                        message::outbound::connect::Done done;

                        // we'll use if the descriptor is valid or not do deduce if the
                        // connection was succesfull or not, from the main thread
                        auto send_done = execute::scope( [&done]()
                           {
                              try
                              {
                                 common::signal::thread::scope::Block block;

                                 log::line( verbose::log, "send main_connect: ", done);
                                 common::communication::ipc::blocking::send( common::communication::ipc::inbound::ipc(), done);
                              }
                              catch( ...)
                              {
                                 common::exception::handle();
                              }
                           }
                        );

                        // try forever to connect to remote
                        communication::tcp::inbound::Device inbound( communication::tcp::retry::connect( address, {
                           { std::chrono::milliseconds{ 100}, 100}, // 10s
                           { std::chrono::seconds{ 1}, 3600}, // 1h
                           { std::chrono::seconds{ 5}, 0} // forever
                        }));

                        // send connect request
                        {
                           Trace trace{ "gateway::outbound::local::connect send domain connect request"};

                           common::message::gateway::domain::connect::Request request;
                           request.domain = common::domain::identity();
                           request.versions = { common::message::gateway::domain::protocol::Version::version_1};
                           
                           log::line( verbose::log, "request: ", request);

                           communication::tcp::outbound::blocking::send( inbound.connector().socket(), request);
                        }

                        // wait for reply
                        common::message::gateway::domain::connect::Reply reply;
                        {
                           Trace trace{ "gateway::outbound::local::connect connect_thread wait for reply"};

                           auto handler = inbound.handler(
                              common::message::handle::assign( reply)
                           );

                           while( ! reply.correlation)
                           {
                              handler( inbound.next( inbound.policy_blocking()));
                           }

                           log::line( verbose::log, "reply: ", reply);

                           if( reply.version != common::message::gateway::domain::protocol::Version::version_1)
                              throw common::exception::system::invalid::Argument{ "invalid protocol"};
                        }

                        // connect to gateway
                        {
                           Trace trace{ "gateway::outbound::local::connect connect_thread send connect to gateway"};

                           message::outbound::Connect connect;
                           connect.process = common::process::handle();
                           connect.domain = reply.domain;
                           connect.version = reply.version;
                           const auto& socket = inbound.connector().socket();
                           connect.address.local = communication::tcp::socket::address::host( socket);
                           connect.address.peer = communication::tcp::socket::address::peer( socket);

                           log::line( verbose::log, "connect: ", connect);

                           common::communication::ipc::blocking::send(
                              common::communication::instance::outbound::gateway::manager::device(),
                              connect);
                        }

                        // we're done, release the responsibility of the socket
                        // and 'done' will be sent to main thread
                        done.descriptor = inbound.connector().socket().release();
                     }
                     catch( const exception::signal::User&)
                     {
                        // we've been asked to shutdown from the main thread.
                     }
                     catch( ...)
                     {
                        // we've already sent the connect::Done to main thread
                        common::exception::handle();
                     }
                  };

                  std::thread worker{ connect_thread, std::move( settings.address)};

                  // make sure we allways join the worker
                  auto worker_join = common::execute::scope( [&worker](){ worker.join();});
                  
                  // We block sig-user so worker always gets'em
                  common::signal::thread::scope::Block block{ { common::signal::Type::user}};

                  try
                  {
                     // wait for the connect
                     auto done = inbound::message< message::outbound::connect::Done>();

                     if( ! done.descriptor)
                        throw exception::system::communication::unavailable::no::Connect{ "failed to connect to remote domain"};

                     return State{ communication::Socket{ done.descriptor}, settings.order};
                  }
                  catch( ...)
                  {
                     // we're shutting down or something went wrong, either way, we shutdown the thread.
                     signal::thread::send( worker, signal::Type::user);
                     throw;
                  }
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

                                    auto now = common::platform::time::clock::type::now();

                                    state.metric.services.emplace_back(
                                          std::move( destination.service),
                                          std::chrono::duration_cast< std::chrono::microseconds>( now - destination.start));

                                    blocking::optional::send( destination.destination.ipc, message);
                                 }
                                 catch( const common::exception::system::invalid::Argument&)
                                 {
                                    log::line( log::category::error, "failed to correlate [", message.correlation, "] reply with a destination - action: ignore");
                                    log::line( verbose::log, "state.service.route: ", state.service.route);
                                 }

                                 // send service metrics if we don't have any more in-flight call request (this one
                                 // was the last, or only) OR we've accumulated enough metrics for a batch update
                                 if( state.service.route.empty() || state.metric.services.size() >= common::platform::batch::gateway::metrics)
                                 {
                                    blocking::send( common::communication::instance::outbound::service::manager::device(), state.metric);
                                    state.metric.services.clear();
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

                                 auto now = common::platform::time::clock::type::now();

                                 if( ! message.flags.exist( common::message::service::call::request::Flag::no_reply))
                                 {
                                    // potentially notify TM that this "resource" is involved in the transaction
                                    blocking::transaction::involved( message);

                                    state.service.route.emplace(
                                       message.correlation,
                                       message.process,
                                       message.service.name,
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

                                    // potentially notify TM that this "resource" is involved in the transaction
                                    blocking::transaction::involved( message);

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

                     auto handler( State& state)
                     {
                        return communication::ipc::inbound::device().handler(
                           
                           common::message::handle::Shutdown{},
                           common::message::handle::ping(),

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

               void configure( State& state)
               {
                  Trace trace{ "gateway::outbound::local::configure"};
                  
                  
                  {
                     message::outbound::configuration::Request request;
                     request.process = common::process::handle();

                     common::communication::ipc::blocking::send(
                           common::communication::instance::outbound::gateway::manager::device(),
                           request);
                  }
                  
                  {
                     Trace trace{ "gateway::outbound::local::configure send discover"};

                     auto configuration = inbound::message< message::outbound::configuration::Reply>();

                     if( ! configuration.services.empty() || ! configuration.queues.empty())
                     {
                        common::message::gateway::domain::discover::Request request;

                        // We make sure we get the reply (hence not forwarding to some other process)
                        request.process = common::process::handle();
                        request.domain = common::domain::identity();
                        request.services = configuration.services;
                        request.queues = configuration.queues;

                        // Use regular handler to manage the "routing"
                        handle::internal::domain::discover::Request{ state}( request);
                     }
                  }
               }

               void start( State&& state)
               {
                  Trace trace{ "gateway::outbound::local::start"};

                  configure( state);

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

               int main( int argc, char* argv[])
               {
                  try
                  {
                     Settings settings;
                     {
                        argument::Parse parse{ "tcp outbound",
                           argument::Option( std::tie( settings.address), { "-a", "--address"}, "address to the remote domain [(ip|domain):]port"),
                           argument::Option( std::tie( settings.order), { "-o", "--order"}, "order of the outbound connector"),
                        };
                         parse( argc, argv);
                     }

                     start( connect( std::move( settings)));
                  }
                  catch( ...)
                  {
                     return exception::handle();
                  }
                  return 0;
               }
            } // <unnamed>
         } // local


      } // outbound
   } // gateway
} // casual

int main( int argc, char* argv[])
{
   return casual::gateway::outbound::local::main( argc, argv);
} // main