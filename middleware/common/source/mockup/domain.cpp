//!
//! domain.cpp
//!
//! Created on: Feb 15, 2015
//!     Author: Lazan
//!

#include "common/mockup/domain.h"

#include "common/flag.h"
#include "common/trace.h"


namespace casual
{
   namespace common
   {
      namespace mockup
      {

         namespace broker
         {

            namespace server
            {
               std::vector< communication::message::Complete> Connect::operator () ( message_type message)
               {
                  Trace trace{ "mockup::broker::server::Connect", log::internal::debug};

                  reply_type reply;
                  reply.correlation = message.correlation;

                  std::vector< communication::message::Complete> result;
                  result.emplace_back( marshal::complete( reply));

                  log::internal::debug << "mockup connect reply - message: " << range::make( result) << '\n';

                  return result;
               }

            } // server

            namespace client
            {
               std::vector< communication::message::Complete> Connect::operator () ( message_type message)
               {
                  Trace trace{ "mockup::broker::client::Connect", log::internal::debug};

                  reply_type reply;

                  reply.correlation = message.correlation;

                  {
                     common::message::transaction::resource::Manager rm;
                     rm.key = "rm-mockup";
                     reply.resources.push_back( std::move( rm));
                  }

                  std::vector< communication::message::Complete> result;
                  result.emplace_back( marshal::complete( reply));
                  return result;
               }

            } // client


            Lookup::Lookup( std::vector< common::message::service::lookup::Reply> replies)
            {
               log::internal::debug << "replies: " << range::make( replies) << "\n";

               for( auto& reply : replies)
               {
                  m_broker.emplace( reply.service.name, std::move( reply));
               }
            }

            std::vector< communication::message::Complete> Lookup::operator () ( message_type message)
            {
               Trace trace{ "mockup::broker::Lookup", log::internal::debug};

               common::message::service::lookup::Reply reply;

               auto found = range::find( m_broker, message.requested);

               if( found)
               {
                  reply = found->second;
                  reply.state = message::service::lookup::Reply::State::idle;
               }
               else
               {
                  reply.state = message::service::lookup::Reply::State::absent;
               }

               reply.correlation = message.correlation;

               std::vector< communication::message::Complete> result;
               result.emplace_back( marshal::complete( reply));
               return result;
            }

            namespace lookup
            {
               std::vector< communication::message::Complete> Process::operator () ( message_type message)
               {
                  Trace trace{ "mockup::broker::lookup::Process", log::internal::debug};

                  static std::map< Uuid, platform::ipc::id::type> mapping{
                     { common::process::instance::transaction::manager::identity(), mockup::ipc::transaction::manager::id()},
                  };

                  auto reply = message::reverse::type( message);
                  reply.process.queue = mapping[ message.identification];

                  std::vector< communication::message::Complete> result;
                  result.emplace_back( marshal::complete( reply));
                  return result;
               }
            }

         } // broker

         namespace service
         {

            Call::Call( std::vector< std::pair< std::string, reply_type>> replies)
            {
               for( auto& reply : replies)
               {
                  m_server.emplace( reply.first, std::move( reply.second));
               }
            }

            std::vector< communication::message::Complete> Call::operator () ( message_type message)
            {
               Trace trace{ "mockup::service::Call", log::internal::debug};

               if( common::flag< TPNOREPLY>( message.flags))
               {
                  //common::log::error << "message.descriptor: " << message.descriptor << " correlation: " << message.correlation << std::endl;
                  return {};
               }

               auto reply = message::reverse::type( message);
               auto found = range::find( m_server, message.service.name);

               if( found)
               {
                  reply = found->second;
               }

               reply.correlation = message.correlation;
               reply.buffer.memory = message.buffer.memory;
               reply.buffer.type = message.buffer.type;
               reply.transaction.trid = message.trid;
               reply.descriptor = message.descriptor;


               std::vector< communication::message::Complete> result;
               result.emplace_back( marshal::complete( reply));
               return result;
            }

         } // service

         namespace transaction
         {

            std::vector< communication::message::Complete> Commit::operator () ( message_type message)
            {
               Trace trace{ "mockup::transaction::Commit", log::internal::debug};

               std::vector< communication::message::Complete> result;

               {
                  common::message::transaction::commit::Reply reply;

                  reply.correlation = message.correlation;
                  reply.process = common::mockup::ipc::transaction::manager::queue().process();
                  reply.state = XA_OK;
                  reply.stage = common::message::transaction::commit::Reply::Stage::prepare;
                  reply.trid = message.trid;

                  result.emplace_back( marshal::complete( reply));

                  reply.stage = common::message::transaction::commit::Reply::Stage::commit;
                  result.emplace_back( marshal::complete( reply));
               }
               return result;
            }

            std::vector< communication::message::Complete> Rollback::operator () ( message_type message)
            {
               Trace trace{ "mockup::transaction::Rollback", log::internal::debug};

               reply_type reply;

               reply.correlation = message.correlation;
               reply.process = common::mockup::ipc::transaction::manager::queue().process();
               reply.state = XA_OK;
               reply.trid = message.trid;

               std::vector< communication::message::Complete> result;
               result.emplace_back( marshal::complete( reply));
               return result;
            }

         } // transaction


         namespace create
         {

            namespace lookup
            {
               common::message::service::lookup::Reply reply(
                     const std::string& service,
                     platform::ipc::id::type queue,
                     std::chrono::microseconds timeout)
               {
                  common::message::service::lookup::Reply reply;
                  reply.process.queue = queue;
                  reply.process.pid = common::process::id();
                  reply.service.name = service;
                  reply.service.timeout = timeout;

                  return reply;

               }

            } // lookup

            transform::Handler broker( std::vector< message::service::lookup::Reply> replies)
            {
               return transform::Handler{
                  broker::Lookup{ std::move( replies)},
                  broker::server::Connect{},
                  broker::client::Connect{},
                  broker::lookup::Process{},
               };
            }

            transform::Handler broker()
            {
               return transform::Handler{ broker::server::Connect{}, broker::client::Connect{}};
            }

            common::mockup::transform::Handler server( std::vector< std::pair< std::string, message::service::call::Reply>> replies)
            {
               return transform::Handler{ service::Call{ std::move( replies)}};
            }

            namespace transaction
            {
               common::mockup::transform::Handler manager()
               {
                  return transform::Handler{ mockup::transaction::Commit{}, mockup::transaction::Rollback{}};
               }
            } // transaction


         } // create

         namespace domain
         {
            namespace local
            {
               namespace
               {
                  template< typename M>
                  reply::result_t result( const process::Handle& process, M&& message)
                  {
                     return { process.queue, std::forward< M>( message)};
                  }

                  template< typename M>
                  std::vector< reply::result_t> result_set( const process::Handle& process, M&& message)
                  {
                     std::vector< reply::result_t> result;
                     result.emplace_back( process.queue, std::forward< M>( message));
                     return result;
                  }

               } // <unnamed>
            } // local

            namespace service
            {
               std::vector< reply::result_t> Echo::operator()( message::service::call::callee::Request request)
               {
                  auto reply = message::reverse::type( request);

                  reply.buffer = std::move( request.buffer);
                  reply.transaction.trid = request.trid;

                  return local::result_set( request.process, std::move( reply));
               }

            } // server

            namespace broker
            {
               Lookup::Lookup( std::vector< common::message::service::lookup::Reply> replies)
               {
                  log::internal::debug << "replies: " << range::make( replies) << "\n";

                  for( auto& reply : replies)
                  {
                     m_services.emplace( reply.service.name, std::move( reply));
                  }
               }

               std::vector< reply::result_t> Lookup::operator()( message::service::lookup::Request request)
               {
                  Trace trace{ "mockup::broker::Lookup", log::internal::debug};

                  auto reply = message::reverse::type( request);

                  auto found = range::find( m_services, request.requested);

                  if( found)
                  {
                     reply = found->second;
                     reply.correlation = request.correlation;
                     reply.state = message::service::lookup::Reply::State::idle;
                  }
                  else
                  {
                     reply.state = message::service::lookup::Reply::State::absent;
                  }

                  return local::result_set( request.process, std::move( reply));
               }

               namespace transaction
               {
                  namespace client
                  {

                     std::vector< reply::result_t> Connect::operator () ( message::transaction::client::connect::Request r)
                     {
                        Trace trace{ "mockup::broker::transaction::client::Connect", log::internal::debug};

                        auto reply = m_reply;
                        reply.correlation = r.correlation;

                        return local::result_set( r.process, std::move( reply));
                     }
                  }
               }

            } // broker



            Broker::Broker( reply::Handler handler, dummy_t)
               : m_replier{ std::move( handler)}
               , m_broker_replier_link{ mockup::ipc::broker::queue().output().connector().id(), m_replier.input()}
            {

            }

            Broker::Broker() : Broker( default_handler())
            {
               //
               // Set up TM identification, so we can use the broker without setting up a mockup-TM.
               // If a real or mockup TM connects this will be overwritten
               //
               m_state.singeltons[ process::instance::transaction::manager::identity()].queue = mockup::ipc::transaction::manager::id();
            }

            Broker::~Broker() = default;

            std::vector< reply::result_t> Broker::check_process_connect( const process::Handle& process)
            {
               Trace trace{ "mockup check_process_connect", log::internal::debug};

               std::vector< reply::result_t> result;

               auto found = m_state.process_request.find( process.pid);

               if( found != std::end( m_state.process_request))
               {
                  for( auto& target : found->second)
                  {
                     log::internal::debug << "mockup - process connect found waiter: " << target.process << " for: " << process << std::endl;

                     auto reply = message::reverse::type( target);
                     reply.process = process;
                     result.push_back( local::result( target.process, reply));
                  }
                  m_state.process_request.erase( found);
               }
               return result;
            }

            reply::Handler Broker::default_handler()
            {

               return reply::Handler{
                  [&]( message::server::connect::Request r)
                  {
                     Trace trace{ "mockup server::connect::Request", log::internal::debug};

                     m_state.servers.push_back( r.process);

                     auto result = check_process_connect( r.process);

                     auto reply = message::reverse::type( r);
                     reply.directive = decltype( reply)::Directive::start;

                     if( r.identification)
                     {
                        auto found = range::find(  m_state.singeltons, r.identification);

                        if( found && found->second != r.process)
                        {
                           log::internal::debug << "mockup process: " << r.process << " is a singleton, and one is already running\n";
                           reply.directive = decltype( reply)::Directive::singleton;
                           return local::result_set( r.process, reply);
                        }
                        else
                        {
                           m_state.singeltons[ r.identification] = r.process;

                           auto found = range::find(  m_state.singelton_request, r.identification);

                           if( found)
                           {
                              for( auto& target : found->second)
                              {
                                 auto reply = message::reverse::type( target);
                                 reply.process = r.process;
                                 result.push_back( local::result( target.process, reply));
                              }
                              m_state.singelton_request.erase( found.begin());
                           }
                        }
                     }

                     for( auto& service : r.services)
                     {
                        auto& reply = m_state.services[ service.name];
                        reply.service = service;
                        reply.process = r.process;

                        log::internal::debug << "mockup - service added: " << reply << std::endl;
                     }

                     result.push_back( local::result( r.process, reply));

                     return result;
                  },
                  [&]( common::message::inbound::ipc::Connect c)
                  {
                     Trace trace{ "mockup inbound::ipc::Connect", log::internal::debug};


                     m_state.servers.push_back( c.process);

                     return check_process_connect( c.process);

                  },
                  [&]( common::message::process::lookup::Request r)
                  {
                     Trace trace{ "mockup lookup::process::Request", log::internal::debug};

                     if( r.identification)
                     {
                        log::internal::debug << "mockup - lockup for identificatin: " << r.identification << '\n';

                        auto found = range::find(  m_state.singeltons, r.identification);

                        if( found)
                        {
                           auto reply = message::reverse::type( r);
                           reply.process = found->second;
                           return local::result_set( r.process, reply);
                        }
                        else
                        {
                           m_state.singelton_request[ r.identification].push_back( std::move( r));
                        }
                     }
                     else if( r.pid)
                     {
                        log::internal::debug << "lockup for pid: " << r.pid << '\n';

                        auto found = range::find_if( m_state.servers, [=]( const process::Handle& h){
                           return h.pid == r.pid;
                        });

                        auto reply = message::reverse::type( r);

                        if( found)
                        {
                           reply.process = *found;
                           log::internal::debug << "found server from pid: " << reply.process << '\n';
                           return local::result_set( r.process, reply);
                        }
                        else if( r.directive == common::message::process::lookup::Request::Directive::wait)
                        {
                           m_state.process_request[ r.pid].push_back( std::move( r));
                        }
                        else
                        {
                           return local::result_set( r.process, reply);
                        }
                     }
                     return std::vector< reply::result_t>{};
                  },
                  []( message::transaction::client::connect::Request r)
                  {

                     Trace trace{ "mockup transaction::client::connect::Request", log::internal::debug};
                     auto reply = message::reverse::type( r);
                     reply.directive = decltype( reply)::Directive::start;

                     return local::result_set( r.process, reply);
                  },
                  []( common::message::transaction::manager::connect::Request r)
                  {
                     Trace trace{ "mockup transaction::manager::connect::Request", log::internal::debug};

                     auto reply = message::reverse::type( r);
                     reply.directive = decltype( reply)::Directive::start;

                     auto result = local::result_set( r.process, reply);

                     common::message::transaction::manager::Configuration conf;
                     conf.correlation = r.correlation;
                     result.push_back( local::result( r.process, conf));

                     return result;
                  },
                  [&]( message::service::lookup::Request r)
                  {
                     Trace trace{ "mockup service::lookup::Request", log::internal::debug};

                     auto found = range::find( m_state.services, r.requested);

                     if( found)
                     {
                        auto reply = found->second;
                        reply.correlation = r.correlation;
                        return local::result_set( r.process, reply);
                     }

                     auto reply = message::reverse::type( r);
                     reply.state = decltype( reply)::State::absent;

                     return local::result_set( r.process, reply);
                  },
                  []( common::message::transaction::manager::Ready r)
                  {
                     Trace trace{ "mockup common::message::transaction::manager::Ready", log::internal::debug};
                     return std::vector< reply::result_t>{};
                  },
                  []( common::message::forward::connect::Request r)
                  {
                     Trace trace{ "mockup forward::connect::Request", log::internal::debug};
                     auto reply = message::reverse::type( r);
                     reply.directive = decltype( reply)::Directive::start;

                     return local::result_set( r.process, reply);
                  },
               };
            }


            namespace transaction
            {
               reply::Handler Manager::default_handler()
               {

                  return reply::Handler{
                     []( common::message::transaction::commit::Request message)
                     {
                        Trace trace{ "mockup::transaction::Commit", log::internal::debug};

                        common::message::transaction::commit::Reply reply;

                        reply.correlation = message.correlation;
                        reply.process = common::mockup::ipc::transaction::manager::queue().process();
                        reply.state = XA_OK;
                        reply.stage = common::message::transaction::commit::Reply::Stage::prepare;
                        reply.trid = message.trid;

                        auto result = local::result_set( message.process, reply);

                        reply.stage = common::message::transaction::commit::Reply::Stage::commit;
                        result.push_back( local::result( message.process, reply));

                        return result;

                     },
                     []( common::message::transaction::rollback::Request message)
                     {
                        Trace trace{ "mockup::transaction::Rollback", log::internal::debug};

                        common::message::transaction::rollback::Reply reply;

                        reply.correlation = message.correlation;
                        reply.process = common::mockup::ipc::transaction::manager::queue().process();
                        reply.state = XA_OK;
                        reply.trid = message.trid;

                        return local::result_set( message.process, reply);
                     }
                  };
               }

               Manager::Manager() : Manager( default_handler()) {}

               Manager::Manager( reply::Handler handler)
                  : m_replier{ std::move( handler)}
                  , m_tm_replier_link{ mockup::ipc::transaction::manager::queue().output().connector().id(), m_replier.input()}
               {

               }

            } // transaction


            Domain::Domain()
               : server1{ reply::Handler{ service::Echo{}}},
                  m_broker{
                  broker::Lookup
                  {
                     {
                       mockup::create::lookup::reply( "service1", server1.input()),
                       mockup::create::lookup::reply( "service2", server1.input()),
                       mockup::create::lookup::reply( "service3_2ms_timout", server1.input(), std::chrono::milliseconds{ 2}),
                       mockup::create::lookup::reply( "removed_ipc_queue", 0),
                     }
                  }}
            {

            }


         } // domain

      } // mockup
   } // common
} // casual
