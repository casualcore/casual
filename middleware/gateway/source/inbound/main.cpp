//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/inbound/buffer.h"
#include "gateway/common.h"
#include "gateway/message.h"

#include "common/communication/instance.h"
#include "common/communication/tcp.h"
#include "common/communication/select.h"
#include "common/argument.h"
#include "common/environment.h"
#include "common/exception/handle.h"
#include "common/execute.h"

#include "common/message/gateway.h"
#include "common/message/handle.h"
#include "common/message/queue.h"
#include "common/message/transaction.h"
#include "common/message/coordinate.h"


namespace casual
{
   using namespace common;

   namespace gateway
   {
      namespace inbound
      {

         namespace local
         {
            namespace
            {
               using size_type = platform::size::type;

               namespace manager
               {
                  auto& service() { return communication::instance::outbound::service::manager::device();}
                  auto& queue() { return common::communication::instance::outbound::queue::manager::optional::device();}
                  
               } // manager


               namespace blocking
               {
                  template< typename D, typename M>
                  void send( D&& device, M&& message)
                  {
                     common::communication::device::blocking::send( std::forward< D>( device), std::forward< M>( message));
                  }


                  namespace optional
                  {
                     template< typename D, typename M>
                     auto send( D&& device, M&& message)
                     {
                        return common::communication::device::blocking::optional::send( std::forward< D>( device), std::forward< M>( message));
                     }
                  } // optional
               } // blocking


               struct Settings
               {
                  strong::socket::id descriptor;

                  struct
                  {
                     size_type size = 0;
                     size_type messages = 0;
                  } limit;

               };

               namespace discover
               {
                  namespace coordinate
                  {
                     // Policy to coordinate discover request from another domain, 
                     // used with common::message::Coordinate
                     struct Policy 
                     {
                        using message_type = common::message::gateway::domain::discover::Reply;

                        inline void operator() ( message_type& message, message_type& reply)
                        {
                           Trace trace{ "gateway::inbound::handle::domain::discover::coordinate::Policy"};

                           common::algorithm::append( reply.services, message.services);
                           common::algorithm::append( reply.queues, message.queues);

                           common::log::line( verbose::log, "message: ", message);
                           common::log::line( verbose::log, "reply: ", reply);
                        }
                     };

                     using Discover = common::message::Coordinate< Policy>;


                     struct Send 
                     {
                        Send( communication::tcp::Duplex& device) : m_device( device) {}

                        template< typename Message>
                        inline void operator() ( common::strong::ipc::id queue, Message& message)
                        {
                           Trace trace{ "gateway::inbound::handle::domain::discover::coordinate::Send"};

                           // Set our domain-id to the reply so the outbound can deduce stuff
                           message.domain = common::domain::identity();
                           message.process = common::process::handle();

                           common::log::line( verbose::log, "sending message: ", message);

                           communication::device::blocking::send( m_device, message);
                        }
                     private:
                        communication::tcp::Duplex& m_device;
                     };


                  } // coordinate
               } // discover

               struct State 
               {
                  State( Settings settings)
                     : external{ communication::Socket{ settings.descriptor}},
                        buffer{ buffer::Limit( settings.limit.size, settings.limit.messages)},
                        coordinate( external.device)
                  {
                  }

                  template< typename M> 
                  void add_message( M&& message)
                  {
                     buffer.add( std::forward< M>( message));

                     if( buffer.congested())
                        directive.read.remove( external.device.connector().socket().descriptor());
                  } 

                  template< typename... Ts>
                  auto get_complete( Ts&&... ts)
                  {
                     auto complete = buffer.get( std::forward< Ts>( ts)...);
                     
                     if( ! buffer.congested())
                        directive.read.add( external.device.connector().socket().descriptor());

                     return complete;
                  }



                  struct 
                  {
                     communication::tcp::Duplex device;

                     template< typename M>
                     auto send( M&& message)
                     {
                        return communication::device::blocking::send( 
                           device,
                           std::forward< M>( message));
                     }

                  } external;

                  Buffer buffer;
                  communication::select::Directive directive;
                  struct Coordinate
                  {
                     Coordinate( communication::tcp::Duplex& device) : send{ device} {}
                     discover::coordinate::Discover discover;
                     discover::coordinate::Send send;

                  } coordinate;
                  

               };

               namespace handle
               {
                  namespace external
                  {
                     namespace call
                     {
                        auto request( State& state)
                        {
                           return [&state]( common::message::service::call::callee::Request& message)
                           {
                              Trace trace{ "gateway::inbound::handle::external::call::Request"};

                              common::log::line( verbose::log, "message: ", message);

                              // Change 'sender' so we get the reply
                              message.process = common::process::handle();

                              // Prepare lookup
                              common::message::service::lookup::Request request;
                              {
                                 request.correlation = message.correlation;
                                 request.requested = message.service.name;
                                 request.context = decltype( request.context)::no_busy_intermediate;
                                 request.process = common::process::handle();
                              }

                              // Add message to buffer
                              state.add_message( std::move( message));

                              // Send lookup
                              blocking::send( local::manager::service(), request);
                           };
                        }
                     } // call

                     namespace queue
                     {
                        namespace lookup
                        {
                           template< typename M>
                           bool send( State& state, M&& message)
                           {
                              Trace trace{ "gateway::inbound::handle::external::queue::lookup::send"};

                              // Change 'sender' so we get the reply
                              message.process = common::process::handle();

                              // Prepare queue lookup
                              common::message::queue::lookup::Request request{ common::process::handle()};
                              {
                                 request.correlation = message.correlation;
                                 request.name = message.name;
                              }

                              // Add message to buffer
                              state.add_message( std::forward< M>( message));

                              auto remove = common::execute::scope( [&](){
                                 state.get_complete( request.correlation);
                              });

                              try
                              {
                                 // Send lookup
                                 blocking::send( manager::queue(), request);

                                 // We could send the lookup, so we won't remove the message from the buffer
                                 remove.release();
                              }
                              catch( ...)
                              {
                                 if( exception::code() == code::casual::communication_unavailable)
                                    return false;

                                 throw;
                              }

                              return true;
                           }
                        } // lookup

                        namespace enqueue
                        {
                           auto request( State& state)
                           {
                              return [&state]( common::message::queue::enqueue::Request& message)
                              {
                                 Trace trace{ "gateway::inbound::handle::external::queue::enqueue::Request"};

                                 common::log::line( verbose::log, "message: ", message);

                                 // Send lookup
                                 if( ! queue::lookup::send( state, message))
                                 {
                                    common::log::line( common::log::category::error, "failed to lookup queue - action: send error reply");

                                    common::message::queue::enqueue::Reply reply;
                                    reply.correlation = message.correlation;
                                    reply.execution = message.execution;

                                    // empty uuid represent error. TODO: is this enough?
                                    reply.id = common::uuid::empty();

                                    state.external.send( reply);
                                 }
                              };
                           }
                        } // enqueue

                        namespace dequeue
                        {
                           auto request( State& state)
                           {
                              return [&]( common::message::queue::dequeue::Request& message)
                              {
                                 Trace trace{ "gateway::inbound::handle::queue::dequeue::Request::operator()"};

                                 common::log::line( verbose::log, "message: ", message);

                                 // Send lookup
                                 if( ! queue::lookup::send( state, message))
                                 {
                                    common::log::line( common::log::category::error, "failed to lookup queue - action: send error reply");

                                    common::message::queue::enqueue::Reply reply;
                                    reply.correlation = message.correlation;
                                    reply.execution = message.execution;

                                    // empty uuid represent error. TODO: is this enough?
                                    reply.id = common::uuid::empty();

                                    blocking::send( common::communication::ipc::inbound::ipc(), reply);
                                 }
                              };
                           }
                        } // dequeue
                     } // queue

                     namespace domain
                     {
                        namespace discover
                        {
                           auto request( State& state)
                           {
                              return [&state]( common::message::gateway::domain::discover::Request& message)
                              {
                                 Trace trace{ "gateway::inbound::handle::external::connection::discover::Request"};

                                 common::log::line( verbose::log, "message: ", message);

                                 // Make sure we gets the reply
                                 message.process = common::process::handle();

                                 // Forward to broker and possible casual-queue

                                 std::vector< common::strong::process::id> pids;

                                 auto& service_manager = local::manager::service();

                                 if( ! message.services.empty())
                                 {
                                    blocking::send( service_manager, message);
                                    pids.push_back( service_manager.connector().process().pid);
                                 }

                                 if( ! message.queues.empty() && blocking::optional::send( manager::queue(), message))
                                    pids.push_back( manager::queue().connector().process().pid);

                                 state.coordinate.discover.add( message.correlation, {}, state.coordinate.send, std::move( pids));
                              };
                           }
                        } // discover
                     } // domain


                     namespace transaction
                     {
                        namespace resource
                        {
                           template< typename Message>
                           auto basic_request()
                           {
                              return []( Message& message)
                              {
                                 // Set 'sender' so we get the reply
                                 message.process = common::process::handle();
                                 blocking::send( common::communication::instance::outbound::transaction::manager::device(), message);
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

                     auto handler( State& state)
                     {
                        return common::message::dispatch::handler( state.external.device,
                              
                           // service call
                           call::request( state),

                           // queue
                           queue::enqueue::request( state),
                           queue::dequeue::request( state),

                           // domain discover
                           domain::discover::request( state),

                           // transaction
                           transaction::resource::prepare::request(),
                           transaction::resource::commit::request(),
                           transaction::resource::rollback::request()
                        );
                     }

                     namespace create
                     {
                        auto dispatch( State& state)
                        {
                           const auto descriptor = state.external.device.connector().descriptor();
                           state.directive.read.add( descriptor);

                           return communication::select::dispatch::create::reader(
                              descriptor,
                              [&device = state.external.device, handler = external::handler( state)]( auto active) mutable
                              {
                                 handler( communication::device::non::blocking::next( device));
                              }
                           );
                        }
                     } // create
                  } // external

                  namespace internal
                  {
                     template< typename Message>
                     auto basic_forward( State& state)
                     {
                        return [&state]( Message& message)
                        {
                           Trace trace{ "gateway::inbound::local::handle::basic_forward"};
                           common::log::line( verbose::log, "forward message: ", message);

                           state.external.send( message);
                        };
                     }

                     namespace call
                     {
                        namespace lookup
                        {
                           auto reply( State& state)
                           {
                              return [&state]( common::message::service::lookup::Reply& message)
                              {
                                 Trace trace{ "gateway::inbound::local::handle::internal::call::lookup::reply"};
                                 common::log::line( verbose::log, "message: ", message);

                                 auto send_error_reply = [&state, &message]( auto code)
                                 {
                                    common::message::service::call::Reply reply;
                                    reply.correlation = message.correlation;
                                    reply.code.result = code;
                                    state.external.send( reply);
                                 };

                                 auto request = state.get_complete( message.correlation, message.pending);

                                 switch( message.state)
                                 {
                                    using Enum = decltype( message.state);
                                    case Enum::idle:
                                    {
                                       if( ! communication::device::blocking::optional::put( message.process.ipc, request))
                                       {
                                          log::line( common::log::category::error, common::code::xatmi::service_error, " server: ", message.process, " has been terminated during interdomain call - action: reply with: ", common::code::xatmi::service_error);
                                          send_error_reply( common::code::xatmi::service_error);
                                       }
                                       break;
                                    }
                                    case Enum::absent:
                                    {
                                       log::line( common::log::category::error, common::code::xatmi::no_entry, " service: ", message.service, " is not handled by this domain (any more) - action: reply with: ", common::code::xatmi::no_entry);
                                       send_error_reply( common::code::xatmi::no_entry);
                                       break;
                                    }
                                    default:
                                    {
                                       log::line( common::log::category::error, common::code::xatmi::service_error, " unexpected state on lookup reply: ", message, " - action: reply with: ", common::code::xatmi::service_error);
                                       send_error_reply( common::code::xatmi::service_error);
                                       break;
                                    }
                                 }
                              };  
                           }

                        } // lookup

                        auto reply = basic_forward< common::message::service::call::Reply>;

                     } // call

                     namespace queue
                     {
                        namespace lookup
                        {
                           auto reply( State& state)
                           {
                              return [&state]( common::message::queue::lookup::Reply& message)
                              {
                                 Trace trace{ "gateway::inbound::local::handle::internal::queue::lookup::reply"};
                                 common::log::line( verbose::log, "message: ", message);


                                 auto request = state.get_complete( message.correlation);

                                 if( message.process)
                                 {
                                    common::communication::ipc::outbound::Device ipc{ message.process.ipc};
                                    ipc.put( request, common::communication::ipc::policy::Blocking{});
                                    return;
                                 }

                                 // queue not available - send error reply
                    
                                 auto send_error = [&state, &message]( auto&& reply)
                                 {
                                    reply.correlation = message.correlation;
                                    state.external.send( reply);
                                 };

                                 switch( request.type)
                                 {
                                    using Enum = decltype( request.type);
                                    case Enum::queue_dequeue_request:
                                    {
                                       send_error( common::message::queue::dequeue::Reply{});
                                       break;
                                    }
                                    case Enum::queue_enqueue_request:
                                    {
                                       send_error( common::message::queue::enqueue::Reply{});
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
                           auto reply = basic_forward< common::message::queue::dequeue::Reply>;
                        } // dequeue

                        namespace enqueue
                        {
                           auto reply = basic_forward< common::message::queue::enqueue::Reply>;
                        } // enqueue
                     } // queue

                     namespace domain
                     {
                        namespace discover
                        {
                           auto reply( State& state)
                           {
                              return [&state]( common::message::gateway::domain::discover::Reply& message)
                              {
                                 Trace trace{ "gateway::inbound::handle::internal::domain::discover::Reply"};
                                 common::log::line( verbose::log, "message: ", message);

                                 // Might send the accumulated message if all requested has replied.
                                 // (via the Policy)
                                 state.coordinate.discover.accumulate( message, state.coordinate.send);
                              };
                           }

                        } // discover
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

                     auto handler( State& state)
                     {
                        auto& device = common::communication::ipc::inbound::device();
                        return common::message::dispatch::handler( device, 
   
                           common::message::handle::defaults( device),

                           // service call
                           call::lookup::reply( state),
                           call::reply( state),

                           // queue
                           queue::lookup::reply( state),
                           queue::dequeue::reply( state),
                           queue::enqueue::reply( state),

                           // transaction
                           transaction::resource::prepare::reply( state),
                           transaction::resource::commit::reply( state),
                           transaction::resource::rollback::reply( state),

                           // domain discovery
                           domain::discover::reply( state)
                        );
                     }
                     namespace create
                     {
                        using handler_type = decltype( common::message::dispatch::handler( communication::ipc::inbound::device()));
                        struct Dispatch
                        {
                           Dispatch( State& state) : m_handler( internal::handler( state)) 
                           {
                              state.directive.read.add( communication::ipc::inbound::handle().socket().descriptor());
                           }

                           auto descriptor() const { return communication::ipc::inbound::handle().socket().descriptor();}

                           auto consume()
                           {
                              return m_handler( communication::device::non::blocking::next( common::communication::ipc::inbound::device()));
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

               void connect( State& state)
               {
                  Trace trace{ "gateway::inbound::local::connect"};

                  common::message::gateway::domain::connect::Request request;
                  
                  // Wait for the other domain to connect.
                  {
                     Trace trace{ "gateway::inbound::local::connect remote domain connect"};
               
                     auto handler = common::message::dispatch::handler( state.external.device,
                        common::message::handle::assign( request)
                     );

                     while( ! request.correlation)
                        handler( communication::device::blocking::next( state.external.device));
                  }

                  using version_type = common::message::gateway::domain::protocol::Version;

                  auto transform = []( const common::message::gateway::domain::connect::Request& request)
                  {
                     auto reply = common::message::reverse::type( request);

                     if( common::algorithm::find( request.versions, version_type::version_1))
                        reply.version = version_type::version_1;

                     reply.domain = common::domain::identity();
                     return reply;
                  };

                  auto reply = transform( request);

                  // send reply
                  {
                     Trace trace{ "gateway::inbound::local::connect connect reply remote domain"};

                     // send reply to other domain
                     communication::device::blocking::send( state.external.device, reply);

                     if( reply.version == version_type::invalid)
                        code::raise::error( code::casual::invalid_version, " no compatable protocol in connect: ", common::range::make( request.versions));
                  }

                  // make sure gateway know we're connected
                  {
                     Trace trace{ "gateway::inbound::local::connect connect gateway"};

                     message::inbound::Connect connect{ common::process::handle()};

                     connect.domain = request.domain;
                     connect.version = reply.version;
                     const auto& socket = state.external.device.connector().socket();
                     connect.address.local = communication::tcp::socket::address::host( socket);
                     connect.address.peer = communication::tcp::socket::address::peer( socket);

                     common::communication::device::blocking::send( common::communication::instance::outbound::gateway::manager::device(), connect);
                  }  
               }

               void start( State&& state)
               {
                  Trace trace{ "gateway::inbound::local::start"};

                  // 'connect' to our local domain
                  common::communication::instance::connect();

                  // connect to the other domain
                  connect( state);

                  log::line( log::category::information, "inbound connected: ", 
                     communication::tcp::socket::address::host( state.external.device.connector().socket()));

                  // start the message dispatch
                  communication::select::dispatch::pump( 
                     state.directive, 
                     handle::internal::create::dispatch( state),
                     handle::external::create::dispatch( state)
                  );
               }

               void main( int argc, char **argv)
               {
                  Settings settings;

                  argument::Parse{ "tcp inbound",
                     argument::Option( std::tie( settings.descriptor.underlaying()), { "--descriptor"}, "socket descriptor"),
                     argument::Option( std::tie( settings.limit.messages), { "--limit-messages"}, "# of concurrent messages"),
                     argument::Option( std::tie( settings.limit.size), { "--limit-size"}, "max size of concurrent messages")
                  }( argc, argv);

                  start( State{ settings});
               }
            } // <unnamed>
         } // local

      } // inbound
   } // gateway

} // casual


int main( int argc, char **argv)
{
   return casual::common::exception::main::guard( [=]()
   {
      casual::gateway::inbound::local::main( argc, argv);
   });
}