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
               using size_type = common::platform::size::type;

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
                           return false;
                        }
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
                     //
                     // Policy to coordinate discover request from another domain, 
                     // used with common::message::Coordinate
                     //
                     struct Policy 
                     {
                        Policy( const communication::Socket& external) : m_external( external) {}

                        using message_type = common::message::gateway::domain::discover::Reply;

                        inline void accumulate( message_type& message, message_type& reply)
                        {
                           Trace trace{ "gateway::inbound::handle::domain::discover::coordinate::Policy::accumulate"};

                           common::algorithm::append( reply.services, message.services);
                           common::algorithm::append( reply.queues, message.queues);

                           common::log::line( verbose::log, "message: ", message);
                           common::log::line( verbose::log, "reply: ", reply);
                        }

                        inline void send( common::strong::ipc::id queue, message_type& message)
                        {
                           Trace trace{ "gateway::inbound::handle::domain::discover::coordinate::Policy::send"};

                           //
                           // Set our domain-id to the reply so the outbound can deduce stuff
                           //
                           message.domain = common::domain::identity();
                           message.process = common::process::handle();

                           common::log::line( verbose::log, "sending message: ", message);

                           communication::tcp::outbound::blocking::send( m_external, message);
                        }
                     private:
                        const communication::Socket& m_external;
                     };

                     using Discover = common::message::Coordinate< Policy>;

                  } // coordinate
               } // discover

               struct State 
               {
                  State( Settings settings)
                     : external{ communication::Socket{ settings.descriptor}},
                        buffer{ buffer::Limit( settings.limit.size, settings.limit.messages)},
                        coordinate( external.inbound.connector().socket())
                  {
                  }

                  template< typename M> 
                  void add_message( M&& message)
                  {
                     buffer.add( common::marshal::complete( std::forward< M>( message)));

                     if( buffer.congested())
                        directive.read.remove( external.inbound.connector().socket().descriptor());
                  } 

                  auto get_complete( const common::Uuid& correlation)
                  {
                     auto complete = buffer.get( correlation);
                     
                     if( ! buffer.congested())
                        directive.read.add( external.inbound.connector().socket().descriptor());

                     return complete;
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

                  Buffer buffer;
                  communication::select::Directive directive;
                  discover::coordinate::Discover coordinate;

               };

               namespace handle
               {
                  namespace state
                  {
                     struct Base
                     {
                        Base( State& state) : m_state( state) {}

                     protected:
                        State& m_state;
                     };
                  } // state

                  namespace external
                  {
                     namespace call
                     {
                        struct Request : state::Base
                        {
                           using message_type = common::message::service::call::callee::Request;

                           using state::Base::Base;


                           void operator() ( message_type& message)
                           {
                              Trace trace{ "gateway::inbound::handle::external::call::Request"};

                              common::log::line( verbose::log, "message: ", message);

                              // Change 'sender' so we (our main thread) get the reply
                              message.process = common::process::handle();

                              // Prepare lookup
                              common::message::service::lookup::Request request;
                              {
                                 request.correlation = message.correlation;
                                 request.requested = message.service.name;
                                 request.context = common::message::service::lookup::Request::Context::gateway;
                                 request.process = common::process::handle();
                              }

                              // Add message to buffer
                              m_state.add_message( std::move( message));

                              // Send lookup
                              blocking::send( common::communication::instance::outbound::service::manager::device(), request);
                           }
                        };
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
                              common::message::queue::lookup::Request request;
                              {
                                 request.correlation = message.correlation;
                                 request.name = message.name;
                                 request.process = common::process::handle();
                              }

                              // Add message to buffer
                              state.add_message( std::move( message));

                              auto remove = common::execute::scope( [&](){
                                 state.get_complete( request.correlation);
                              });

                              try
                              {
                                 // Send lookup
                                 blocking::send( common::communication::instance::outbound::queue::manager::optional::device(), request);

                                 // We could send the lookup, so we won't remove the message from the buffer
                                 remove.release();
                              }
                              catch( const common::exception::system::communication::Unavailable&)
                              {
                                 return false;
                              }

                              return true;
                           }
                        } // lookup

                        namespace enqueue
                        {
                           struct Request : state::Base
                           {
                              using message_type = common::message::queue::enqueue::Request;

                              using state::Base::Base;

                              void operator() ( message_type& message) const
                              {
                                 Trace trace{ "gateway::inbound::handle::external::queue::enqueue::Request"};

                                 common::log::line( verbose::log, "message: ", message);

                                 // Send lookup
                                 if( ! queue::lookup::send( m_state, message))
                                 {
                                    common::log::line( common::log::category::error, "failed to lookup queue - action: send error reply");

                                    common::message::queue::enqueue::Reply reply;
                                    reply.correlation = message.correlation;
                                    reply.execution = message.execution;

                                    // empty uuid represent error. TODO: is this enough?
                                    reply.id = common::uuid::empty();

                                    m_state.external.send( reply);
                                 }
                              }
                           };
                        } // enqueue

                        namespace dequeue
                        {
                           struct Request : state::Base
                           {
                              using message_type = common::message::queue::dequeue::Request;

                              using state::Base::Base;

                              void operator() ( message_type& message) const
                              {
                                 Trace trace{ "gateway::inbound::handle::queue::dequeue::Request::operator()"};

                                 common::log::line( verbose::log, "message: ", message);

                                 // Send lookup
                                 if( ! queue::lookup::send( m_state, message))
                                 {
                                    common::log::line( common::log::category::error, "failed to lookup queue - action: send error reply");

                                    common::message::queue::enqueue::Reply reply;
                                    reply.correlation = message.correlation;
                                    reply.execution = message.execution;

                                    // empty uuid represent error. TODO: is this enough?
                                    reply.id = common::uuid::empty();

                                    blocking::send( common::communication::ipc::inbound::ipc(), reply);
                                 }
                              }
                           };
                        } // dequeue
                     } // queue

                     namespace domain
                     {
                        namespace discover
                        {
                           struct Request : state::Base
                           {
                              using state::Base::Base;


                              void operator () ( common::message::gateway::domain::discover::Request& message) const
                              {
                                 Trace trace{ "gateway::inbound::handle::external::connection::discover::Request"};

                                 common::log::line( verbose::log, "message: ", message);

                                 // Make sure we gets the reply
                                 message.process = common::process::handle();

                                 //
                                 // Forward to broker and possible casual-queue

                                 std::vector< common::strong::process::id> pids;

                                 auto& service_manager = common::communication::instance::outbound::service::manager::device();

                                 if( ! message.services.empty())
                                 {
                                    blocking::send( service_manager, message);
                                    pids.push_back( service_manager.connector().process().pid);
                                 }

                                 auto& queue_manager = common::communication::instance::outbound::queue::manager::optional::device();

                                 if( ! message.queues.empty() &&
                                       blocking::optional::send( queue_manager, message))
                                 {
                                    pids.push_back( queue_manager.connector().process().pid);
                                 }

                                 m_state.coordinate.add( message.correlation, {}, std::move( pids));
                              }
                           };
                           
                        } // discover
                     } // domain


                     namespace transaction
                     {
                        namespace resource
                        {
                           template< typename M>
                           struct basic_request
                           {
                              using message_type = M;

                              void operator() ( message_type& message)
                              {
                                 // Set 'sender' so we get the reply
                                 message.process = common::process::handle();
                                 blocking::send( common::communication::instance::outbound::transaction::manager::device(), message);
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

                     auto handler( State& state)
                     {
                        return state.external.inbound.handler(
                              
                           // service call
                           call::Request{ state},

                           // queue
                           queue::enqueue::Request{ state},
                           queue::dequeue::Request{ state},

                           // domain discover
                           domain::discover::Request{ state},

                           // transaction
                           transaction::resource::prepare::Request{},
                           transaction::resource::commit::Request{},
                           transaction::resource::rollback::Request{}
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
                     struct basic_forward : state::Base
                     {
                        using state::Base::Base;

                        using message_type = M;

                        void operator() ( message_type& message)
                        {
                           Trace trace{ "gateway::inbound::handle::basic_forward::operator()"};

                           common::log::line( verbose::log, "forward message: ", message);

                           m_state.external.send( message);
                        }
                     };

                     namespace call
                     {
                        namespace lookup
                        {
                           struct Reply : state::Base
                           {
                              using message_type = common::message::service::lookup::Reply;

                              using state::Base::Base;

                              void operator() ( message_type& message)
                              {
                                 common::log::line( verbose::log, "message: ", message);

                                 auto request = m_state.get_complete( message.correlation);

                                 switch( message.state)
                                 {
                                    case message_type::State::idle:
                                    {
                                       try
                                       {
                                          common::communication::ipc::outbound::Device ipc{ message.process.ipc};
                                          ipc.put( request, common::communication::ipc::policy::Blocking{});
                                       }
                                       catch( const common::exception::system::communication::Unavailable&)
                                       {
                                          log::line( common::log::category::error, "server: ", message.process, " has been terminated during interdomain call - action: reply with TPESVCERR");
                                          send_error_reply( message);
                                       }
                                       break;
                                    }
                                    case message_type::State::absent:
                                    {
                                       log::line( common::log::category::error, "service: ", message.service, " is not handled by this domain (any more) - action: reply with TPESVCERR");
                                       send_error_reply( message);
                                       break;
                                    }
                                    default:
                                    {
                                       log::line( common::log::category::error, "unexpected state on lookup reply: ", message, " - action: drop message");
                                       break;
                                    }
                                 }
                              }

                           private:
                              void send_error_reply( message_type& message)
                              {
                                 common::message::service::call::Reply reply;
                                 reply.correlation = message.correlation;
                                 reply.code.result = common::code::xatmi::service_error;
                                 m_state.external.send( reply);
                              }
                           };

                        } // lookup

                        using Reply = basic_forward< common::message::service::call::Reply>;

                     } // call

                     namespace queue
                     {
                        namespace lookup
                        {
                           struct Reply : state::Base
                           {
                              using message_type = common::message::queue::lookup::Reply;
                              using state::Base::Base;


                              void operator() ( message_type& message) const
                              {
                                 Trace trace{ "gateway::inbound::handle::internal::queue::lookup::Reply"};

                                 common::log::line( verbose::log, "message: ", message);

                                 auto request = m_state.get_complete( message.correlation);

                                 if( message.process)
                                 {
                                    common::communication::ipc::outbound::Device ipc{ message.process.ipc};
                                    ipc.put( request, common::communication::ipc::policy::Blocking{});
                                 }
                                 else
                                 {
                                    send_error_reply( message, request.type);
                                 }
                              }

                           private:

                              template< typename R>
                              void send_error_reply( message_type& message) const
                              {
                                 R reply;
                                 reply.correlation = message.correlation;
                                 m_state.external.send( reply);
                              }

                              void send_error_reply( message_type& message, common::message::Type type) const
                              {
                                 switch( type)
                                 {
                                    case common::message::Type::queue_dequeue_request:
                                    {
                                       send_error_reply< common::message::queue::dequeue::Reply>( message);
                                       break;
                                    }
                                    case common::message::Type::queue_enqueue_request:
                                    {
                                       send_error_reply< common::message::queue::enqueue::Reply>( message);
                                       break;
                                    }
                                    default:
                                    {
                                       common::log::line( common::log::category::error, "unexpected message type for queue request: ", message, " - action: drop message");
                                    }
                                 }
                              }

                           };
                        } // lookup

                        namespace dequeue
                        {
                           using Reply = basic_forward< common::message::queue::dequeue::Reply>;
                        } // dequeue

                        namespace enqueue
                        {
                           using Reply = basic_forward< common::message::queue::enqueue::Reply>;
                        } // enqueue
                     } // queue

                     namespace domain
                     {
                        namespace discover
                        {
                           struct Reply : state::Base
                           {
                              using state::Base::Base;
                              using message_type = common::message::gateway::domain::discover::Reply;

                              void operator() ( message_type& message)
                              {
                                 Trace trace{ "gateway::inbound::handle::internal::domain::discover::Reply"};

                                 common::log::line( log, "message: ", message);

                                 //
                                 // Might send the accumulated message if all requested has replied.
                                 // (via the Policy)
                                 //
                                 m_state.coordinate.accumulate( message);
                              }
                           };

                        } // discover
                     } // domain


                     namespace transaction
                     {
                        namespace resource
                        {
                           namespace prepare
                           {
                              using Reply = basic_forward< common::message::transaction::resource::prepare::Reply>;
                           } // prepare
                           namespace commit
                           {
                              using Reply = basic_forward< common::message::transaction::resource::commit::Reply>;
                           } // commit
                           namespace rollback
                           {
                              using Reply = basic_forward< common::message::transaction::resource::rollback::Reply>;
                           } // commit
                        } // resource
                     } // transaction

                     auto handler( State& state)
                     {
                        return common::communication::ipc::inbound::device().handler(
   
                           common::message::handle::Shutdown{},
                           common::message::handle::ping(),

                           // service call
                           call::lookup::Reply{ state},
                           call::Reply( state),

                           // queue
                           queue::lookup::Reply{ state},
                           queue::dequeue::Reply( state),
                           queue::enqueue::Reply( state),

                           // transaction
                           transaction::resource::prepare::Reply( state),
                           transaction::resource::commit::Reply( state),
                           transaction::resource::rollback::Reply( state),

                           // domain discovery
                           domain::discover::Reply{ state}
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

               void connect( State& state)
               {
                  Trace trace{ "gateway::inbound::local::connect"};

                  common::message::gateway::domain::connect::Request request;
                  
                  // Wait for the other domain to connect.
                  {
                     Trace trace{ "gateway::inbound::local::connect remote domain connect"};
               
                     auto handler = state.external.inbound.handler(
                        common::message::handle::assign( request)
                     );

                     while( ! request.correlation)
                     {
                        handler( state.external.inbound.next( state.external.inbound.policy_blocking()));
                     }
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
                     communication::tcp::outbound::blocking::send( state.external.inbound.connector().socket(), reply);

                     if( reply.version == version_type::invalid)
                     {
                        throw common::exception::system::invalid::Argument{ common::string::compose( "no compatable protocol in connect: ", common::range::make( request.versions))};
                     }
                  }

                  // make sure gateway know we're connected
                  {
                     Trace trace{ "gateway::inbound::local::connect connect gateway"};

                     message::inbound::Connect connect;

                     connect.domain = request.domain;
                     connect.process = common::process::handle();
                     connect.version = reply.version;
                     const auto& socket = state.external.inbound.connector().socket();
                     connect.address.local = communication::tcp::socket::address::host( socket);
                     connect.address.peer = communication::tcp::socket::address::peer( socket);

                     common::communication::ipc::blocking::send( common::communication::instance::outbound::gateway::manager::device(), connect);
                  }  
               }

               void start( State&& state)
               {
                  // 'connect' to our local domain
                  common::communication::instance::connect();

                  // connect to the other domain
                  connect( state);

                  log::line( log::category::information, "inbound connected: ", 
                     communication::tcp::socket::address::host( state.external.inbound.connector().socket()));

                  // start the message dispatch
                  communication::select::dispatch::pump( 
                     state.directive, 
                     handle::internal::create::dispatch( state),
                     handle::external::create::dispatch( state)
                  );
               }

               int main( int argc, char **argv)
               {
                  try
                  {
                     Settings settings;
                     {
                        argument::Parse parse{ "tcp inbound",
                           argument::Option( std::tie( settings.descriptor.underlaying()), { "--descriptor"}, "socket descriptor"),
                           argument::Option( std::tie( settings.limit.messages), { "--limit-messages"}, "# of concurrent messages"),
                           argument::Option( std::tie( settings.limit.size), { "--limit-size"}, "max size of concurrent messages")
                        };
                        parse( argc, argv);
                     }

                     start( State{ settings});
                  }
                  catch( ...)
                  {
                     return exception::handle();
                  }
                  return 0;
               }
            } // <unnamed>
         } // local

      } // inbound
   } // gateway

} // casual


int main( int argc, char **argv)
{
   return casual::gateway::inbound::local::main( argc, argv);
}