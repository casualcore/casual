//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/communication/instance.h"
#include "common/communication/ipc.h"
#include "common/communication/log.h"

#include "common/message/domain.h"
#include "common/message/server.h"
#include "common/environment.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

namespace casual
{
   namespace common
   {
      namespace communication
      {

         namespace instance
         {
         
            namespace fetch
            {
               namespace local
               {
                  namespace
                  {
                     auto call( common::message::domain::process::lookup::Request& request)
                     {
                        // We create a temporary inbound, so we don't rely on the 'global' inbound
                        communication::ipc::inbound::Device inbound;

                        request.process.pid = common::process::id();
                        request.process.ipc = inbound.connector().handle().ipc();

                        return communication::ipc::call(
                              outbound::domain::manager::device(),
                              request,
                              inbound).process;
                     }
                  } // <unnamed>
               } // local

               std::ostream& operator << ( std::ostream& out, Directive directive)
               {
                  switch( directive)
                  {
                     case Directive::wait: return out << "wait";
                     case Directive::direct: return out << "direct";
                  }
                  return out << "unknown";
               }

               process::Handle handle( const Uuid& identity, Directive directive)
               {
                  Trace trace{ "common::communication::instance::fetch::handle"};

                  common::log::line( log, "identity: ", identity, ", directive: ", directive);

                  common::message::domain::process::lookup::Request request;
                  request.directive = static_cast< common::message::domain::process::lookup::Request::Directive>( directive);
                  request.identification = identity;

                  return local::call( request);
               }

               process::Handle handle( strong::process::id pid , Directive directive)
               {
                  Trace trace{ "common::communication::instance::fetch::handle (pid)"};

                  log::line( log::debug, "pid: ", pid, ", directive: ", directive);

                  common::message::domain::process::lookup::Request request;
                  request.directive = static_cast< common::message::domain::process::lookup::Request::Directive>( directive);
                  request.pid = pid;

                  return local::call( request);
               }

            } // fetch

            namespace local
            {
               namespace
               {
                  template< typename M>
                  void connect_reply( M&& message)
                  {
                     switch( message.directive)
                     {
                        case M::Directive::singleton:
                        {
                           code::raise::error( code::casual::shutdown, "domain-manager denied startup - reason: executable is a singleton - action: terminate");
                        }
                        case M::Directive::shutdown:
                        {
                           code::raise::error( code::casual::shutdown, "domain-manager denied startup - reason: domain-manager is in shutdown mode - action: terminate");
                        }
                        default:
                        {
                           break;
                        }
                     }
                  }

                  template< typename M>
                  void connect( M&& message)
                  {
                     signal::thread::scope::Mask block{ signal::set::filled( code::signal::terminate, code::signal::interrupt)};

                     connect_reply( communication::ipc::call( outbound::domain::manager::device(), message));
                  }

               } // <unnamed>
            } // local

            void connect( const Uuid& identity, const process::Handle& process)
            {
               Trace trace{ "communication::instance::connect identity"};

               common::message::domain::process::connect::Request request;
               request.identification = identity;
               request.correlation = identity;
               request.process = process;

               local::connect( request);
            }

            void connect( const Uuid& identity)
            {
               connect( identity, process::handle());
            }

            void connect( const process::Handle& process)
            {
               Trace trace{ "communication::instance::connect process"};

               common::message::domain::process::connect::Request request;
               request.process = process;

               local::connect( request);
            }

            void connect()
            {
               connect( process::handle());
            }


            process::Handle ping( strong::ipc::id queue)
            {
               Trace trace{ "communication::instance::ping"};

               common::message::server::ping::Request request;
               request.process = process::handle();

               return communication::ipc::call( queue, request).process;
            }

            namespace outbound
            {
               namespace detail
               {
                  namespace local
                  {
                     namespace
                     {

                        process::Handle fetch(
                              const Uuid& identity,
                              const std::string& environment,
                              fetch::Directive directive)
                        {
                           Trace trace{ "communication::instance::outbound::instance::local::fetch"};

                           log::line( verbose::log, "identity: ", identity, ", environment: ", environment, ", directive: ", directive);

                           if( common::environment::variable::exists( environment))
                           {
                              auto process = environment::variable::process::get( environment);

                              log::line( verbose::log, "process: ", process);

                              if( ipc::exists( process.ipc))
                                 return process;
                           }

                           try
                           {
                              auto process = instance::fetch::handle( identity, directive);

                              if( process && ! environment.empty())
                                 environment::variable::process::set( environment, process);

                              return process;
                           }
                           catch( ...)
                           {
                              if( exception::code() == code::casual::communication_unavailable)
                              {
                                 common::log::line( log, "failed to fetch instance with identity: ", identity);
                                 return {};
                              }
                              throw;
                           }
                        }

                     } // <unnamed>
                  } // local

                  template< fetch::Directive directive>
                  basic_connector< directive>::basic_connector( const Uuid& identity, std::string environment)
                     : base_connector( local::fetch( identity, environment, directive)),
                       m_identity{ identity}, m_environment{ std::move( environment)}
                  {
                     log::line( log, "instance created - ", m_environment, " ipc: ", m_process.ipc);
                     log::line( verbose::log, "connector: ", *this);
                  }


                  template< fetch::Directive directive>
                  void basic_connector< directive>::reconnect()
                  {
                     Trace trace{ "communication::instance::outbound::Connector::reconnect"};

                     reset( local::fetch( m_identity, m_environment, directive));

                     if( ! m_process)
                        code::raise::generic( code::casual::communication_unavailable, verbose::log, "process absent: ", m_process);

                     log::line( verbose::log, "connector: ", *this);
                  }

                  template< fetch::Directive directive>
                  void basic_connector< directive>::clear()
                  {
                     Trace trace{ "communication::instance::outbound::Connector::clear"};

                     environment::variable::unset( m_environment);
                     m_connector.clear();
                  }

                  template struct basic_connector< fetch::Directive::direct>;
                  template struct basic_connector< fetch::Directive::wait>;
               } // detail

               namespace service
               {
                  namespace manager
                  {
                     detail::Device& device()
                     {
                        static detail::Device singelton{
                           identity::service::manager,
                           common::environment::variable::name::ipc::service::manager};

                        return singelton;
                     }

                  } // manager
               } // service


               namespace transaction
               {
                  namespace manager
                  {
                     detail::Device& device()
                     {
                        static detail::Device singelton{
                           identity::transaction::manager,
                           common::environment::variable::name::ipc::transaction::manager};
                        return singelton;
                     }

                  } // manager
               } // transaction

               namespace gateway
               {
                  namespace manager
                  {
                     detail::Device& device()
                     {
                        static detail::Device singelton{
                           identity::gateway::manager,
                           environment::variable::name::ipc::gateway::manager};

                        return singelton;
                     }

                     namespace optional
                     {
                        detail::optional::Device& device()
                        {
                           static detail::optional::Device singelton{
                              identity::gateway::manager,
                              environment::variable::name::ipc::gateway::manager};

                           return singelton;
                        }
                     } // optional

                  } // manager
               } // gateway

               namespace queue
               {
                  namespace manager
                  {
                     detail::Device& device()
                     {
                        static detail::Device singelton{
                           identity::queue::manager,
                           environment::variable::name::ipc::queue::manager};

                        return singelton;
                     }

                     namespace optional
                     {
                        detail::optional::Device& device()
                        {
                           static detail::optional::Device singelton{
                              identity::queue::manager,
                              environment::variable::name::ipc::queue::manager};

                           return singelton;
                        }
                     } // optional
                  } // manager
               } // queue


               namespace domain
               {
                  namespace manager    
                  {                     
                     namespace local
                     {
                        namespace
                        {

                           template< typename R>
                           process::Handle reconnect( R&& singleton_policy)
                           {
                              Trace trace{ "communication::instance::outbound::domain::manager::local::reconnect"};

                              auto from_environment = []()
                              {
                                 if( environment::variable::exists( environment::variable::name::ipc::domain::manager))
                                    return environment::variable::process::get( environment::variable::name::ipc::domain::manager);

                                 return process::Handle{};
                              };

                              auto process = from_environment();

                              common::log::line( verbose::log, "process: ", process);


                              if( ipc::exists( process.ipc))
                              {
                                 return process;
                              }

                              common::log::line( log, "failed to locate domain manager via ", environment::variable::name::ipc::domain::manager, " - trying 'singleton file'");

                              process = singleton_policy();

                              if( ! ipc::exists( process.ipc))
                                 code::raise::log( code::casual::domain_unavailable, "failed to locate domain manager");

                              return process;
                           }

                        } // <unnamed>
                     } // local

                     Connector::Connector() 
                        : detail::base_connector{ local::reconnect( [](){ return common::domain::singleton::read().process;})}
                     {
                        log::line( verbose::log, "connector: ", *this);
                     }

                     void Connector::reconnect()
                     {
                        Trace trace{ "communication::instance::outbound::domain::manager::Connector::reconnect"};

                        reset( local::reconnect( [](){ return common::domain::singleton::read().process;}));

                        log::line( verbose::log, "connector: ", *this);
                     }

                     void Connector::clear()
                     {
                        Trace trace{ "communication::instance::outbound::domain::manager::Connector::clear"};
                        environment::variable::unset( environment::variable::name::ipc::domain::manager);
                        m_connector.clear();
                     }


                     Device& device()
                     {
                        static Device singelton;
                        return singelton;
                     }

                     namespace optional
                     {
                        Connector::Connector()
                        : detail::base_connector{ local::reconnect( [](){
                           return common::domain::singleton::read().process;
                        })}
                        {
                           log::line( verbose::log, "connector: ", *this);
                        }

                        void Connector::reconnect()
                        {
                           Trace trace{ "communication::instance::outbound::domain::manager::optional::Connector::reconnect"};

                           reset( local::reconnect( [](){
                              return common::domain::singleton::read().process;
                           }));

                           log::line( verbose::log, "connector: ", *this);
                        }

                        void Connector::clear()
                        {
                           Trace trace{ "communication::instance::outbound::domain::manager::optional::Connector::clear"};
                           environment::variable::unset( environment::variable::name::ipc::domain::manager);
                           m_connector.clear();
                        }

                        Device& device()
                        {
                           static Device singelton;
                           return singelton;
                        }
                     } // optional

                  } // manager 

               } // domain
            } // outbound
         } // instance
      } // communication
   } // common
   
} // casual