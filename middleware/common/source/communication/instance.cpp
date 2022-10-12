//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/instance.h"
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
   namespace common::communication::instance
   {
      std::ostream& operator << ( std::ostream& out, const Identity& value)
      {
         stream::write( out, "{ id: ", value.id, ", environment: ", value.environment, "}");
         return out;
      }
   
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
            request.directive = directive == Directive::wait ? decltype( request.directive)::wait : decltype( request.directive)::direct;
            request.identification = identity;

            return local::call( request);
         }

         process::Handle handle( strong::process::id pid , Directive directive)
         {
            Trace trace{ "common::communication::instance::fetch::handle (pid)"};

            log::line( log::debug, "pid: ", pid, ", directive: ", directive);

            common::message::domain::process::lookup::Request request;
            request.directive = directive == Directive::wait ? decltype( request.directive)::wait : decltype( request.directive)::direct;
            request.pid = pid;

            return local::call( request);
         }

      } // fetch

      namespace local
      {
         namespace
         {

            template< typename M>
            void connect( M&& message)
            {
               Trace trace{ "communication::instance::local::connect"};
               log::line( verbose::log, "message: ", message);

               signal::thread::scope::Mask block{ signal::set::filled( code::signal::terminate, code::signal::interrupt)};

               auto directive = communication::ipc::call( outbound::domain::manager::device(), message).directive;

               if( directive != decltype( directive)::approved)
                  code::raise::error( code::casual::shutdown, "domain-manager denied startup - directive: ", directive, " - action: terminate");
            }

         } // <unnamed>
      } // local


      void connect( const process::Handle& process)
      {
         Trace trace{ "communication::instance::connect process"};
         local::connect( common::message::domain::process::connect::Request{ process});
      }

      void connect()
      {
         connect( process::handle());
      }

      namespace whitelist
      {
         void connect()
         {
            common::message::domain::process::connect::Request request{ process::handle()};
            request.whitelist = true;
            request.information.alias = common::instance::alias();
            request.information.path = common::process::path();
            local::connect( request);

         }

         void connect( const instance::Identity& identity, const process::Handle& process)
         {
            Trace trace{ "communication::instance::whitelist::connect identity"};

            common::message::domain::process::connect::Request request{ process};
            request.whitelist = true;
            request.singleton.identification = identity.id;
            request.singleton.environment = identity.environment;
            request.information.alias = common::instance::alias();
            request.information.path = common::process::path();

            local::connect( request);
         }

         void connect( const instance::Identity& identity)
         {
            connect( identity, process::handle());
         }

         void connect( const Uuid& id)
         {
            connect( instance::Identity{ id, {}});
         }

      } // whitelist


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
                        const instance::Identity& identity,
                        fetch::Directive directive)
                  {
                     Trace trace{ "communication::instance::outbound::instance::local::fetch"};

                     log::line( verbose::log, "identity: ", identity, ", directive: ", directive);

                     if( common::environment::variable::exists( identity.environment))
                     {
                        auto process = environment::variable::process::get( identity.environment);

                        log::line( verbose::log, "process: ", process);

                        if( ipc::exists( process.ipc))
                           return process;
                     }

                     try
                     {
                        auto process = instance::fetch::handle( identity.id, directive);

                        if( process && ! identity.environment.empty())
                           environment::variable::process::set( identity.environment, process);

                        return process;
                     }
                     catch( ...)
                     {
                        if( exception::capture().code() != code::casual::communication_unavailable)
                           throw;

                        common::log::line( log, "failed to fetch instance with identity: ", identity);
                        return {};
                     }
                  }

               } // <unnamed>
            } // local

            base_connector::base_connector()
               : m_socket{ ipc::native::detail::create::domain::socket()}
            {}

            void base_connector::reset( process::Handle process)
            {
               m_process = std::move( process);
               m_connector = ipc::outbound::Connector{ m_process.ipc};
            }

            template< fetch::Directive directive>
            basic_connector< directive>::basic_connector( instance::Identity identity)
               : m_identity{ identity}
            {
               log::line( log, "instance created - identity: ", m_identity);
               log::line( verbose::log, "connector: ", *this);
            }

            template< fetch::Directive directive>
            const process::Handle& basic_connector< directive>::process() 
            { 
               if( ! m_process)
                  reconnect();

               return m_process;
            }


            template< fetch::Directive directive>
            void basic_connector< directive>::reconnect()
            {
               Trace trace{ "communication::instance::outbound::Connector::reconnect"};

               reset( local::fetch( m_identity, directive));

               if( ! m_process)
                  code::raise::error( code::casual::communication_unavailable, "process absent: ", m_process);

               log::line( verbose::log, "connector: ", *this);
            }

            template< fetch::Directive directive>
            void basic_connector< directive>::clear()
            {
               Trace trace{ "communication::instance::outbound::Connector::clear"};

               environment::variable::unset( m_identity.environment);
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
                  static detail::Device singelton{ identity::service::manager};
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
                  static detail::Device singelton{ identity::transaction::manager};
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
                  static detail::Device singelton{ identity::gateway::manager};
                  return singelton;
               }

               namespace optional
               {
                  detail::optional::Device& device()
                  {
                     static detail::optional::Device singelton{ identity::gateway::manager};
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
                  static detail::Device singelton{ identity::queue::manager};
                  return singelton;
               }

               namespace optional
               {
                  detail::optional::Device& device()
                  {
                     static detail::optional::Device singelton{ identity::queue::manager};
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
                     process::Handle connect( R&& singleton_policy)
                     {
                        Trace trace{ "communication::instance::outbound::domain::manager::local::connect"};

                        auto from_environment = []()
                        {
                           if( environment::variable::exists( environment::variable::name::ipc::domain::manager))
                              return environment::variable::process::get( environment::variable::name::ipc::domain::manager);

                           return process::Handle{};
                        };

                        auto process = from_environment();

                        if( process.ipc)
                        {
                           common::log::line( verbose::log, "process: ", process);
   
                           if( ipc::exists( process.ipc))
                           {
                              return process;
                           }

                           common::log::line( log, "failed to locate via ", environment::variable::name::ipc::domain::manager);
                        }

                        process = singleton_policy();

                        if( process.ipc)
                        {
                           common::log::line( verbose::log, "process: ", process);

                           if( ipc::exists( process.ipc))
                           {
                              return process;
                           }

                           common::log::line( log, "failed to locate via 'singleton file'");
                        }

                        code::raise::error( code::casual::domain_unavailable, "failed to locate domain manager");
                     }

                  } // <unnamed>
               } // local


               void Connector::reconnect()
               {
                  Trace trace{ "communication::instance::outbound::domain::manager::Connector::reconnect"};

                  reset( local::connect( [](){ return common::domain::singleton::read().process;}));

                  log::line( verbose::log, "connector: ", *this);
               }

               const process::Handle& Connector::process()
               {
                  if( ! m_process)
                     reconnect();
                  return m_process;
               }

               void Connector::clear()
               {
                  Trace trace{ "communication::instance::outbound::domain::manager::Connector::clear"};
                  environment::variable::unset( environment::variable::name::ipc::domain::manager);
                  m_connector.clear();
                  m_process = {};
               }


               Device& device()
               {
                  static Device singelton;
                  return singelton;
               }

               namespace optional
               {
                  void Connector::reconnect()
                  {
                     Trace trace{ "communication::instance::outbound::domain::manager::optional::Connector::reconnect"};

                     reset( local::connect( [](){
                        return common::domain::singleton::read().process;
                     }));

                     log::line( verbose::log, "connector: ", *this);
                  }

                  const process::Handle& Connector::process()
                  {
                     if( ! m_process)
                        reconnect();
                     return m_process;
                  }

                  void Connector::clear()
                  {
                     Trace trace{ "communication::instance::outbound::domain::manager::optional::Connector::clear"};
                     environment::variable::unset( environment::variable::name::ipc::domain::manager);
                     m_connector.clear();
                     m_process = {};
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

   } // common::communication::instance
   
} // casual