//!
//! casual
//!

#include "common/mockup/domain.h"
#include "common/mockup/log.h"

#include "common/environment.h"
#include "common/message/traffic.h"
#include "common/message/gateway.h"



#include "common/flag.h"
#include "common/trace.h"


#include <fstream>

namespace casual
{
   namespace common
   {
      namespace mockup
      {


         namespace domain
         {

            namespace service
            {
               void Echo::operator()( message::service::call::callee::Request& request)
               {
                  Trace trace{ "mockup domain::service::Echo"};

                  if( ! common::flag< TPNOREPLY>( request.flags))
                  {
                     auto reply = message::reverse::type( request);

                     reply.buffer = std::move( request.buffer);
                     reply.transaction.trid = request.trid;
                     reply.descriptor = request.descriptor;

                     if( range::search( request.service.name, std::string{ "urcode"}))
                     {
                        reply.code = 42;
                     }


                     ipc::eventually::send( request.process.queue, reply);
                  }
                  else
                  {
                     log << "TPNOREPLY was set, no echo\n";
                  }
               }

            } // server

            namespace local
            {
               namespace
               {
                  void prepare_domain_manager( platform::ipc::id::type ipc)
                  {
                     common::environment::variable::set( environment::variable::name::ipc::domain::manager(), ipc);

                     log << environment::variable::name::ipc::domain::manager() << " set to: "
                           << common::environment::variable::get( environment::variable::name::ipc::domain::manager()) << '\n';

                     std::ofstream file{ common::environment::domain::singleton::file()};

                     if( file)
                     {
                        file << ipc;
                        log << "ipc: " << ipc << " to: " << common::environment::domain::singleton::file() << '\n';
                     }
                     else
                     {
                        log::error << "faild to create mockup domain singleton file: " << common::environment::domain::singleton::file() << '\n';
                     }
                  }


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
               } // <unnamed>
            } // local

            Manager::Manager() : Manager( default_handler())
            {


            }

            Manager::Manager( message::dispatch::Handler&& handler)
               : m_replier{ std::move( handler)}
            {
               local::prepare_domain_manager( m_replier.input());
            }

            Manager::~Manager() = default;

            message::dispatch::Handler Manager::default_handler()
            {

               return message::dispatch::Handler{
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
                  [&]( message::domain::process::termination::Registration& m)
                  {
                     Trace trace{ "mockup domain process::termination::Registration"};
                  },
               };
            }


            namespace local
            {
               namespace
               {

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


            Broker::Broker() : Broker( default_handler())
            {
            }

            Broker::Broker( message::dispatch::Handler&& handler)
               : m_replier{ std::move( handler)}
            {
               //
               // Connect to the domain
               //
               local::send::connect::domain( m_replier, process::instance::identity::broker());

               //
               // Set environment variable to make it easier for other processes to
               // reach this broker (should work any way...)
               //
               environment::variable::set( environment::variable::name::ipc::broker(), m_replier.input());

            }

            Broker::~Broker() = default;


            message::dispatch::Handler Broker::default_handler()
            {


               return message::dispatch::Handler{

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
                  [&]( message::service::Advertise& m)
                  {
                     Trace trace{ "mockup service::Advertise"};

                     log  << "message: " << m << '\n';

                     for( auto& service : m.services)
                     {
                        auto& lookup = m_state.services[ service.name];
                        lookup.service.name = service.name;
                        lookup.service.type = service.type;
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
                        lookup.service.type = service.type;
                        lookup.service.transaction = service.transaction;
                        lookup.process = m.process;
                     }

                     //log << "services: " << range::make( m_state.services) << '\n';
                  },
                  [&]( message::traffic::monitor::connect::Request& m)
                  {
                     m_state.traffic_monitors.push_back( m.process.queue);
                     ipc::eventually::send( m.process.queue, message::reverse::type( m));
                  },
                  [&]( message::traffic::monitor::Disconnect& m)
                  {
                     range::trim( m_state.traffic_monitors, range::remove( m_state.traffic_monitors, m.process.queue));
                  },
                  [&]( message::gateway::domain::discover::internal::Request& m)
                  {
                     Trace trace{ "mockup gateway::domain::discover::Request"};

                     log << "request: " << m << '\n';

                     auto reply = message::reverse::type( m);

                     reply.domain = m.domain;

                     for( auto&& s : m.services)
                     {
                        auto found = range::find( m_state.services, s);

                        if( found)
                        {
                           traits::concrete::type_t< decltype( reply.services.front())> service;

                           service.name = found->second.service.name;
                           service.timeout = found->second.service.timeout;
                           service.transaction = found->second.service.transaction;
                           service.type = found->second.service.type;

                           reply.services.push_back( std::move( service));
                        }
                     }

                     log << "reply: " << reply << '\n';

                     ipc::eventually::send( m.process.queue, reply);

                  },
                  local::handle::connect::Reply{ "broker"},

               };



            }

            namespace transaction
            {
               message::dispatch::Handler Manager::default_handler()
               {

                  return message::dispatch::Handler{
                     [&]( common::message::transaction::commit::Request& message)
                     {
                        Trace trace{ "mockup::transaction::Commit"};

                        common::message::transaction::commit::Reply reply;

                        reply.correlation = message.correlation;
                        reply.process = m_replier.process();
                        reply.state = XA_OK;
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
                        reply.state = XA_OK;
                        reply.trid = message.trid;

                        ipc::eventually::send( message.process.queue, reply);
                     },
                     local::handle::connect::Reply{ "transaction manager"},

                  };
               }

               Manager::Manager() : Manager( default_handler()) {}

               Manager::Manager( message::dispatch::Handler handler)
                  : m_replier{ std::move( handler)}
               {

                  //
                  // Connect to the domain
                  //
                  local::send::connect::domain( m_replier, process::instance::identity::transaction::manager());
               }

            } // transaction

            namespace queue
            {
               Broker::Broker() : Broker( default_handler())
               {


               }

               Broker::Broker( message::dispatch::Handler&& handler)
                  : m_replier{ std::move( handler)}
               {
                  //
                  // Connect to the domain
                  //
                  local::send::connect::domain( m_replier, process::instance::identity::queue::broker());

                  //
                  // Set environment variable to make it easier for other processes to
                  // reach this broker (should work any way...)
                  //
                  environment::variable::set( environment::variable::name::ipc::queue::broker(), m_replier.input());
               }

               message::dispatch::Handler Broker::default_handler()
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

                        Broker::Message message{ r.message};
                        message.timestamp = platform::clock_type::now();
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

                     communication::ipc::blocking::send( communication::ipc::broker::device(), advertise);

                  }
               } // <unnamed>
            } // local

            namespace echo
            {
               namespace create
               {
                  message::service::advertise::Service service(
                        std::string name,
                        std::chrono::microseconds timeout)
                  {
                     message::service::advertise::Service result;

                     result.name = std::move( name);
                     result.timeout = timeout;

                     return result;
                  }
               } // create




               Server::Server( std::vector< message::service::advertise::Service> services)
                : m_replier{ message::dispatch::Handler{ service::Echo{}, local::handle::connect::Reply{ "echo server"},}}
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

                  communication::ipc::blocking::send( communication::ipc::broker::device(), unadvertise);
               }

               void Server::send_ack( std::string service) const
               {
                  message::service::call::ACK message;
                  message.service = std::move( service);
                  message.process = process();

                  communication::ipc::blocking::send( communication::ipc::broker::device(), message);
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
                     echo::create::service( "service2"),
                     echo::create::service( "service3_2ms_timout", std::chrono::milliseconds{ 2})}}
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
