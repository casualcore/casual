//!
//! casual
//!

#include "common/mockup/domain.h"
#include "common/mockup/log.h"

#include "common/environment.h"
#include "common/message/domain.h"
#include "common/message/event.h"
#include "common/message/gateway.h"
#include "common/message/transaction.h"
#include "common/message/queue.h"

#include "common/event/dispatch.h"

#include "common/flag.h"


#include <fstream>

namespace casual
{
   namespace common
   {
      namespace mockup
      {


         namespace domain
         {
            namespace local
            {
               namespace
               {
                  namespace handle
                  {
                     namespace connect
                     {
                        struct Reply
                        {
                           Reply( std::string information) : information{ std::move( information)} {}

                           void operator () ( message::domain::process::connect::Reply& r)
                           {
                              log << "mockup " << information << " - connected to the domain\n";
                           }

                           std::string information;
                        };
                     } // connect
                  } // handle

                  namespace send
                  {
                     namespace connect
                     {
                        void domain( const mockup::ipc::Replier& replier)
                        {

                           message::domain::process::connect::Request request;
                           request.process = replier.process();

                           communication::ipc::blocking::send( communication::ipc::domain::manager::device(), request);
                        }

                        void domain( const mockup::ipc::Replier& replier, const Uuid& identity)
                        {
                           message::domain::process::connect::Request request;
                           request.identification = identity;
                           request.process = replier.process();

                           communication::ipc::blocking::send( communication::ipc::domain::manager::device(), request);
                        }
                     } // connect
                  } // send
               } // <unnamed>
            } // local

            namespace service
            {

               namespace local
               {
                  namespace
                  {
                     // using limit as a "success"
                     constexpr auto success = code::xatmi::limit;

                     auto reply_error( const std::string& service)
                     {
                        if( range::search( service, std::string{ "TPEOS"})) { return code::xatmi::os;}
                        if( range::search( service, std::string{ "TPEPROTO"})) { return code::xatmi::protocol;}
                        if( range::search( service, std::string{ "TPESVCERR"})) { return code::xatmi::service_error;}
                        if( range::search( service, std::string{ "TPESVCFAIL"})) { return code::xatmi::service_fail;}
                        if( range::search( service, std::string{ "TPESYSTEM"})) { return code::xatmi::system;}
                        if( range::search( service, std::string{ "SUCCESS"})) { return success;}
                        return code::xatmi::ok;
                     }

                  } // <unnamed>
               } // local

               struct Echo
               {
                  void operator()( message::service::call::callee::Request& request)
                  {
                     Trace trace{ "mockup domain::service::Echo"};

                     if( ! request.flags.exist( message::service::call::request::Flag::no_reply))
                     {
                        auto reply = message::reverse::type( request);

                        reply.buffer = std::move( request.buffer);
                        reply.transaction.trid = request.trid;

                        if( range::search( request.service.name, std::string{ "urcode"}))
                        {
                           reply.code = 42;
                        }

                        reply.status = local::reply_error( request.service.name);

                        ipc::eventually::send( request.process.queue, reply);
                     }
                     else
                     {
                        log << "TPNOREPLY was set, no echo\n";
                     }
                  }
               };

               namespace conversation
               {
                  namespace local
                  {
                     namespace
                     {
                        struct State
                        {
                           common::process::Handle process;
                           common::message::conversation::Route route;
                        };

                        flag::service::conversation::Events event_from_error( code::xatmi error)
                        {
                           switch( error)
                           {
                              case service::local::success: return flag::service::conversation::Event::service_success;
                              case code::xatmi::service_fail: return flag::service::conversation::Event::service_fail;
                              default: return flag::service::conversation::Event::service_error;
                           }
                        }

                     } // <unnamed>
                  } // local
                  dispatch_type echo()
                  {
                     auto state = std::make_shared< local::State>();

                     return dispatch_type{
                        [=]( message::mockup::thread::Process& message){
                           Trace trace{ "mockup message::mockup::thread::Process"};

                           state->process = message.process;

                           log << "process: " << state->process << '\n';
                        },
                        [=]( message::conversation::connect::callee::Request& request){
                           Trace trace{ "message::conversation::connect::callee::Request"};

                           log << "message: " << request << '\n';

                           state->route = request.recording;



                           // emulate errors from the service
                           const auto error = service::local::reply_error( request.service.name);

                           log << "mockup: error based on service name: " << error << '\n';

                           if( request.flags & flag::service::conversation::connect::Flag::receive_only)
                           {
                              message::conversation::caller::Send message{ request.buffer};
                              message.correlation = request.correlation;
                              message.execution = request.execution;
                              message.process = state->process;
                              message.route = request.recording;

                              if( error != code::xatmi::ok)
                              {
                                 message.events = local::event_from_error( error);
                              }
                              else
                              {
                                 message.events = flag::service::conversation::Event::send_only;
                              }

                              auto node = message.route.next();

                              log << "send request: " << message << '\n';

                              ipc::eventually::send( node.address, message);
                           }
                           else if( error != code::xatmi::ok)
                           {
                              message::conversation::Disconnect message;
                              message.correlation = request.correlation;
                              message.execution = request.execution;
                              message.process = state->process;
                              message.route = request.recording;
                              message.events = local::event_from_error( error);

                              auto node = message.route.next();

                              ipc::eventually::send( node.address, message);
                           }

                           //
                           // send the reply. This is actually sent before any other messages,
                           // but it's easier to unittest if we know that the potentially replies
                           // above is in the cache (at the caller site), hence we don't have to
                           // worry about 'polling' for the replies, or some other glitches...
                           {
                              auto reply = message::reverse::type( request);

                              reply.process = state->process;
                              reply.recording.nodes.emplace_back( reply.process.queue);
                              reply.route = state->route;

                              auto node = reply.route.next();

                              log << "send connect reply: " << reply << '\n';

                              ipc::eventually::send( node.address, reply);
                           }
                        },
                        [=]( message::conversation::callee::Send& request){
                           Trace trace{ "message::conversation::send::callee::Request"};

                           log << "message: " << request << '\n';

                           if( request.events & flag::service::conversation::Event::send_only)
                           {
                              // echo the message
                              request.route = state->route;
                              request.process = state->process;
                              auto node = request.route.next();

                              ipc::eventually::send( node.address, request);
                           }
                        },
                        [=]( message::conversation::Disconnect& message){
                           Trace trace{ "mockup message::conversation::Disconnect"};

                           log << "message: " << message << '\n';
                        }

                     };
                  }

               } // conversation


               struct Manager::Implementation
               {

                  Implementation( dispatch_type&& handler)
                     : m_replier{  default_handler() + std::move( handler)}
                  {
                     Trace trace{ "mockup domain::Manager::Implementation::Implementation"};

                     //
                     // Connect to the domain
                     //
                     domain::local::send::connect::domain( m_replier, process::instance::identity::service::manager());

                     //
                     // Set environment variable to make it easier for other processes to
                     // reach this broker (should work any way...)
                     //
                     common::environment::variable::process::set(
                           common::environment::variable::name::ipc::service::manager(),
                           m_replier.process());

                     //
                     // Make sure we're up'n running before we let unittest-stuff interact with us...
                     //
                     process::instance::fetch::handle( process::instance::identity::service::manager());
                     }

                  Implementation() : Implementation( dispatch_type{}) {}

                  struct State
                  {
                     std::map< std::string, common::message::service::lookup::Reply> services;
                     std::vector< communication::ipc::Handle> traffic_monitors;
                  };


                  dispatch_type default_handler()
                  {
                     return dispatch_type{

                        [&]( message::service::lookup::Request& r)
                        {
                           Trace trace{ "mockup service::lookup::Request"};

                           log  << "request: " << r << '\n';

                           auto reply = message::reverse::type( r);

                           auto found = range::find( m_state.services, r.requested);

                           if( found)
                           {
                              reply = found->second;
                              reply.correlation = r.correlation;
                              reply.execution = r.execution;
                           }
                           else
                           {
                              reply.service.name = r.requested;
                              reply.state = decltype( reply)::State::absent;
                           }

                           log  << "reply: " << reply << '\n';

                           ipc::eventually::send( r.process.queue, reply);
                        },
                        [&]( message::service::call::ACK& m)
                        {
                           Trace trace{ "mockup service::call::ACK"};

                        },
                        [&]( message::domain::process::prepare::shutdown::Request& m)
                        {
                           Trace trace{ "mockup domain::process::prepare::shutdown::Request"};

                           auto reply = message::reverse::type( m);
                           reply.processes = std::move( m.processes);

                           ipc::eventually::send( m.process.queue, reply);
                        },
                        [&]( message::service::Advertise& m)
                        {
                           Trace trace{ "mockup service::Advertise"};

                           log  << "message: " << m << '\n';

                           for( auto& service : m.services)
                           {
                              auto& lookup = m_state.services[ service.name];
                              lookup.service.name = service.name;
                              lookup.service.category = service.category;
                              lookup.service.transaction = service.transaction;
                              lookup.process = m.process;
                           }
                           //log << "services: " << range::make( m_state.services) << '\n';
                        },
                        [&]( message::gateway::domain::Advertise& m)
                        {
                           Trace trace{ "mockup gateway::domain::service::Advertise"};

                           log  << "message: " << m << '\n';


                           for( auto& service : m.services)
                           {
                              auto& lookup = m_state.services[ service.name];
                              lookup.service.name = service.name;
                              lookup.service.category = service.category;
                              lookup.service.transaction = service.transaction;
                              lookup.process = m.process;
                           }

                           //log << "services: " << range::make( m_state.services) << '\n';
                        },
                        [&]( message::event::subscription::Begin& m)
                        {
                           m_state.traffic_monitors.push_back( m.process.queue);
                        },
                        [&]( message::event::subscription::End& m)
                        {
                           range::trim( m_state.traffic_monitors, range::remove( m_state.traffic_monitors, m.process.queue));
                        },
                        [&]( message::gateway::domain::discover::Request& m)
                        {
                           Trace trace{ "mockup gateway::domain::discover::Request"};

                           log << "request: " << m << '\n';

                           auto reply = message::reverse::type( m);

                           reply.domain = m.domain;
                           reply.process = m_replier.process();

                           for( auto&& s : m.services)
                           {
                              auto found = range::find( m_state.services, s);

                              if( found)
                              {
                                 traits::concrete::type_t< decltype( reply.services.front())> service;

                                 service.name = found->second.service.name;
                                 service.timeout = found->second.service.timeout;
                                 service.transaction = found->second.service.transaction;
                                 service.category = found->second.service.category;

                                 reply.services.push_back( std::move( service));
                              }
                           }

                           log << "reply: " << reply << '\n';

                           ipc::eventually::send( m.process.queue, reply);

                        },
                        domain::local::handle::connect::Reply{ "broker"},

                     };
                  }

                  State m_state;
                  ipc::Replier m_replier;

               };


               Manager::Manager() = default;


               Manager::Manager( dispatch_type&& handler)
                  : m_implementation{ std::move( handler)}
               {


               }

               Manager::~Manager() = default;


            } // service

            struct Manager::Implementation
            {
               Implementation( message::domain::configuration::Domain domain, dispatch_type&& handler, const common::domain::Identity& identity)
                   : m_state{ std::move( domain)},
                     m_replier{ default_handler() + std::move( handler)},
                     m_singlton{ common::domain::singleton::create( m_replier.process(), identity)}
               {
               }

               Implementation( dispatch_type&& handler, const common::domain::Identity& identity)
               : Implementation( {}, std::move( handler), identity)
                 {

                 }


               dispatch_type default_handler()
               {

                  return dispatch_type{
                     [&]( message::domain::process::connect::Request& r)
                     {
                        Trace trace{ "mockup domain::process::connect::Request"};

                        m_state.executables.push_back( r.process);

                        auto reply = message::reverse::type( r);
                        reply.directive = decltype( reply)::Directive::start;

                        if( r.identification)
                        {
                           auto found = range::find(  m_state.singeltons, r.identification);

                           if( found && found->second != r.process)
                           {
                              log  << "mockup process: " << r.process << " is a singleton, and one is already running\n";
                              reply.directive = decltype( reply)::Directive::singleton;

                              ipc::eventually::send( r.process.queue, reply);

                              return;
                           }
                           m_state.singeltons[ r.identification] = r.process;
                        }

                        ipc::eventually::send( r.process.queue, reply);

                        //
                        // Check pending
                        //
                        range::trim( m_state.pending, range::remove_if( m_state.pending, [&]( const common::message::domain::process::lookup::Request& p)
                              {
                                 if( ( p.identification && p.identification == r.identification)
                                       || ( p.pid && p.pid == r.process.pid))
                                 {
                                    log << "mockup found pending: " << p << " for connected process: " << r.process << "\n";

                                    auto reply = message::reverse::type( p);
                                    reply.process = r.process;
                                    ipc::eventually::send( p.process.queue, reply);
                                    return true;
                                 }
                                 return false;
                              }));

                     },
                     [&]( common::message::domain::configuration::Request& r)
                     {
                        Trace trace{ "mockup common::message::domain::configuration::Request"};

                        auto reply = message::reverse::type( r);
                        reply.domain = m_state.configuration;

                        ipc::eventually::send( r.process.queue, reply);
                     },
                     [&]( common::message::domain::configuration::server::Request& r)
                     {
                        Trace trace{ "mockup common::message::domain::configuration::server::Request"};

                        auto reply = message::reverse::type( r);
                        ipc::eventually::send( r.process.queue, reply);
                     },

                     [&]( common::message::domain::process::lookup::Request& r)
                     {
                        Trace trace{ "mockup domain::process::lookup::Request"};

                        auto reply = message::reverse::type( r);

                        if( r.identification)
                        {
                           auto found = range::find(  m_state.singeltons, r.identification);

                           if( found)
                           {
                              log << "mockup - lockup for identity: " << r.identification << ", process: " << found->second << '\n';

                              reply.process = found->second;
                              ipc::eventually::send( r.process.queue, reply);
                              return;
                           }
                        }
                        else if( r.pid)
                        {
                           auto found = range::find_if( m_state.executables, [=]( const process::Handle& h){
                              return h.pid == r.pid;
                           });

                           if( found)
                           {

                              reply.process = *found;
                              log << "mockup - lockup for pid: " << r.pid << ", process: " << reply.process << '\n';
                              ipc::eventually::send( r.process.queue, reply);
                              return;
                           }
                        }

                        // not found
                        if( r.directive == common::message::domain::process::lookup::Request::Directive::wait)
                        {
                           log << "mockup - lockup wait with request: " << r << '\n';
                           m_state.pending.push_back( std::move( r));
                        }
                        else
                        {
                           ipc::eventually::send( r.process.queue, reply);
                        }
                     },
                     [&]( common::message::event::subscription::Begin& m)
                     {
                        Trace trace{ "mockup common::message::event::subscription::Begin"};

                        m_state.events.subscription( m);
                     },
                     [&]( common::message::event::subscription::End& m)
                     {
                        Trace trace{ "mockup common::message::event::subscription::End"};

                        m_state.events.subscription( m);
                     },
                     [&]( common::message::event::process::Exit& m)
                     {
                        Trace trace{ "mockup common::message::event::process::Exit"};

                        m_state.events( m);
                     },
                     [&]( common::message::event::domain::Error& m)
                     {
                        Trace trace{ "mockup common::message::event::domain::Error"};

                        m_state.events( m);
                     },

                  };
               }

               struct State
               {
                  State( message::domain::configuration::Domain domain) : configuration( std::move( domain)) {}
                  State() = default;

                  std::map< common::Uuid, common::process::Handle> singeltons;
                  std::vector< common::message::domain::process::lookup::Request> pending;
                  std::vector< common::process::Handle> executables;


                  common::event::dispatch::Collection<
                     common::message::event::process::Exit,
                     common::message::event::process::Spawn,
                     common::message::event::domain::Error,
                     common::message::event::domain::server::Connect
                  > events;



                  message::domain::configuration::Domain configuration;
               };

               State m_state;
               ipc::Replier m_replier;
               common::file::scoped::Path m_singlton;
            };

            Manager::Manager() : Manager( dispatch_type{})
            {


            }

            Manager::Manager( dispatch_type&& handler, const common::domain::Identity& identity)
               : m_implementation{ std::move( handler), identity}
            {

            }

            Manager::Manager( message::domain::configuration::Domain domain)
               : m_implementation{ std::move( domain), dispatch_type{}, common::domain::Identity{ "unittest-domain"}}
            {

            }

            Manager::~Manager() = default;

            process::Handle Manager::process() const
            {
               return m_implementation->m_replier.process();
            }






            namespace transaction
            {
               dispatch_type Manager::default_handler()
               {

                  return dispatch_type{
                     [&]( common::message::transaction::commit::Request& message)
                     {
                        Trace trace{ "mockup::transaction::Commit"};

                        common::message::transaction::commit::Reply reply;

                        reply.correlation = message.correlation;
                        reply.process = m_replier.process();
                        reply.state = code::tx::ok;
                        reply.stage = common::message::transaction::commit::Reply::Stage::prepare;
                        reply.trid = message.trid;

                        ipc::eventually::send( message.process.queue, reply);

                        reply.stage = common::message::transaction::commit::Reply::Stage::commit;
                        ipc::eventually::send( message.process.queue, reply);

                     },
                     [&]( common::message::transaction::rollback::Request& message)
                     {
                        Trace trace{ "mockup::transaction::Rollback"};

                        common::message::transaction::rollback::Reply reply;

                        reply.correlation = message.correlation;
                        reply.process = m_replier.process();
                        reply.state = code::tx::ok;;
                        reply.trid = message.trid;

                        ipc::eventually::send( message.process.queue, reply);
                     },
                     local::handle::connect::Reply{ "transaction manager"},

                  };
               }

               Manager::Manager() : Manager( dispatch_type{}) {}


               Manager::Manager( dispatch_type&& handler)
                  : m_replier{ default_handler() + std::move( handler)}
               {

                  //
                  // Connect to the domain
                  //
                  local::send::connect::domain( m_replier, process::instance::identity::transaction::manager());

                  //
                  // Set environment variable to make it easier for other processes to
                  // reach TM (should work any way...)
                  //
                  common::environment::variable::process::set(
                        common::environment::variable::name::ipc::transaction::manager(),
                        m_replier.process());

                  //
                  // Make sure we're up'n running before we let unittest-stuff interact with us...
                  //
                  process::instance::fetch::handle( process::instance::identity::transaction::manager());
               }

            } // transaction

            namespace queue
            {
               struct Manager::Implementation
               {
                  Implementation() : Implementation{ dispatch_type{}} {}
                  Implementation( dispatch_type&& handler)
                     : m_replier{ default_handler() + std::move( handler)}
                  {
                     //
                     // Connect to the domain
                     //
                     local::send::connect::domain( m_replier, process::instance::identity::queue::manager());

                     //
                     // Set environment variable to make it easier for other processes to
                     // reach this broker (should work any way...)
                     //
                     environment::variable::set( environment::variable::name::ipc::queue::manager(), m_replier.input());

                     //
                     // Make sure we're up'n running before we let unittest-stuff interact with us...
                     //
                     process::instance::fetch::handle( process::instance::identity::queue::manager());
                  }

                  dispatch_type default_handler()
                  {
                     return {
                        [&]( message::queue::lookup::Request& r)
                        {
                           Trace trace{ "mockup::queue::lookup::Request"};

                           auto reply = message::reverse::type( r);
                           reply.process = m_replier.process();
                           reply.queue = 42;

                           ipc::eventually::send( r.process.queue, reply);
                        },
                        [&]( message::queue::enqueue::Request& r)
                        {
                           Trace trace{ "mockup::queue::enqueue::Request"};

                           auto reply = message::reverse::type( r);
                           reply.id = uuid::make();

                           message::queue::dequeue::Reply::Message message{ r.message};
                           message.timestamp = platform::time::clock::type::now();
                           message.id = reply.id;

                           m_queues[ r.name].push( std::move( message));

                           ipc::eventually::send( r.process.queue, reply);
                        },
                        [&]( message::queue::dequeue::Request& r)
                        {
                           Trace trace{ "mockup::queue::dequeue::Request"};

                           auto reply = message::reverse::type( r);

                           auto& queue = m_queues[ r.name];

                           if( ! queue.empty())
                           {
                              reply.message.push_back( std::move( queue.front()));
                              queue.pop();
                           }

                           ipc::eventually::send( r.process.queue, reply);
                        }
                     };
                  }

                  std::unordered_map< std::string, std::queue< message::queue::dequeue::Reply::Message>> m_queues;
                  ipc::Replier m_replier;
               };

               Manager::Manager() = default;
               Manager::Manager( dispatch_type&& handler) : m_implementation{ std::move( handler)} {}
               Manager::~Manager() = default;

            } // queue




            namespace local
            {
               namespace
               {
                  void advertise( std::vector< message::service::advertise::Service> services, const process::Handle& process)
                  {
                     message::service::Advertise advertise;
                     advertise.process = process;
                     advertise.services = std::move( services);

                     communication::ipc::blocking::send( communication::ipc::service::manager::device(), advertise);

                  }
               } // <unnamed>
            } // local

            namespace echo
            {
               namespace create
               {
                  message::service::advertise::Service service( std::string name)
                  {
                     message::service::advertise::Service result;

                     result.name = std::move( name);

                     return result;
                  }
               } // create




               Server::Server( std::vector< message::service::advertise::Service> services)
                : m_replier{ dispatch_type{
                   service::Echo{},
                   local::handle::connect::Reply{ "echo server"},
                   service::conversation::echo()
                }}
               {
                  //
                  // Connect to the domain
                  //
                  local::send::connect::domain( m_replier);

                  advertise( std::move( services));
               }

               Server::Server( message::service::advertise::Service service)
                  : Server{ std::vector< message::service::advertise::Service>{ std::move( service)}}
               {

               }


               void Server::advertise( std::vector< message::service::advertise::Service> services) const
               {
                  Trace trace{ "mockup echo::Server::advertise"};

                  //
                  // Advertise services
                  //
                  local::advertise( std::move( services),  m_replier.process());
               }

               void Server::undadvertise( std::vector< std::string> services) const
               {
                  Trace trace{ "mockup echo::Server::unadvertise"};

                  message::service::Advertise unadvertise;
                  unadvertise.directive = message::service::Advertise::Directive::remove;
                  unadvertise.process = m_replier.process();
                  range::copy( services, std::back_inserter( unadvertise.services));

                  communication::ipc::blocking::send( communication::ipc::service::manager::device(), unadvertise);
               }

               void Server::send_ack() const
               {
                  message::service::call::ACK message;
                  message.process = process();

                  communication::ipc::blocking::send( communication::ipc::service::manager::device(), message);
               }

               process::Handle Server::process() const
               {
                  return m_replier.process();
               }


            } // echo

            namespace minimal
            {
               Domain::Domain()
                  : server{ {
                     echo::create::service( "service1"),
                     echo::create::service( "service2")}}
               {
                  //
                  // Advertise "removed" ipc queue
                  //
                  process::Handle process{ mockup::pid::next()};
                  local::advertise( { { "removed_ipc_queue"}}, process);
               }

            } // minimal




         } // domain

      } // mockup
   } // common
} // casual
