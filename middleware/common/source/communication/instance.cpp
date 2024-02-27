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

#include "casual/assert.h"

namespace casual
{
   namespace common::communication::instance
   {
      namespace lookup
      {
         namespace local
         {
            namespace
            {

               template< typename F>
               auto send( Directive directive, F callback, const process::Handle& caller = process::handle())
               {
                  auto transform_directive = []( Directive directive)
                  {
                     switch( directive)
                     {
                        using Enum = common::message::domain::process::lookup::request::Directive;
                        case Directive::direct: return Enum::direct;
                        case Directive::wait: return Enum::wait;
                     }
                     casual::terminate( "invalid lookup directive: ", std::to_underlying( directive));
                  };

                  common::message::domain::process::lookup::Request request{ caller};
                  request.directive = transform_directive( directive);
                  // let caller mutate
                  callback( request);

                  return device::blocking::send( outbound::domain::manager::device(), request);
               }
            } // <unnamed>
         } // local
         
         std::string_view description( Directive directive) noexcept
         {
            switch( directive)
            {
               case Directive::wait: return "wait";
               case Directive::direct: return "direct";
            }
            return "unknown";
         }

         strong::correlation::id request( const Uuid& identity, Directive directive)
         {
            Trace trace{ "common::communication::instance::lookup::request"};
            common::log::line( log, "identity: ", identity, ", directive: ", directive);

            return local::send( directive, [ &identity]( auto& request)
            {
               request.identification = identity;
            });
         }

         strong::correlation::id request( strong::process::id pid, Directive directive)
         {
            Trace trace{ "common::communication::instance::fetch::handle (pid)"};
            log::line( log::debug, "pid: ", pid, ", directive: ", directive);

            return local::send( directive, [ pid]( auto& request)
            {
               request.pid = pid;
            });
         }

      } // lookup


      namespace fetch
      {
         namespace local
         {
            namespace
            {
               template< typename F>
               auto call( Directive directive, F callback)
               {
                  // We create a temporary inbound, so we don't rely on the 'global' inbound.
                  // Why? Only because we're using threads in unittest that consumes _global inbound_
                  // TODO: Do we use the  _global inbound_ in threads any more? If, not remove this.
                  communication::ipc::inbound::Device inbound;

                  auto caller = process::Handle{ process::id(), inbound.connector().handle().ipc()};

                  return device::receive< common::message::domain::process::lookup::Reply>(
                     inbound,
                     lookup::local::send( directive, std::move( callback), caller)).process;

               }
            } // <unnamed>
         } // local

         process::Handle handle( const Uuid& identity, Directive directive)
         {
            Trace trace{ "common::communication::instance::fetch::handle"};
            common::log::line( log, "identity: ", identity, ", directive: ", directive);
            
            return local::call( directive, [ &identity]( auto& request)
            {
               request.identification = identity;
            });
         }

         process::Handle handle( strong::process::id pid, Directive directive)
         {
            Trace trace{ "common::communication::instance::fetch::handle (pid)"};
            log::line( log::debug, "pid: ", pid, ", directive: ", directive);

            return local::call( directive, [ pid]( auto& request)
            {
               request.pid = pid;
            });
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

         common::message::domain::process::connect::Request request;
         request.information.handle = process;
         local::connect( request);
      }

      void connect()
      {
         connect( process::handle());
      }

      namespace whitelist
      {
         void connect()
         {
            common::message::domain::process::connect::Request request;
            request.whitelist = true;
            request.information.handle = process::handle();
            request.information.alias = common::instance::alias();
            request.information.path = common::process::path();
            
            local::connect( request);

         }

         void connect( const instance::Identity& identity, const process::Handle& process)
         {
            Trace trace{ "communication::instance::whitelist::connect identity"};

            common::message::domain::process::connect::Request request;
            request.whitelist = true;
            request.singleton.identification = identity.id;
            request.singleton.environment = identity.environment;
            request.information.handle = process;
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


      process::Handle ping( strong::ipc::id id)
      {
         Trace trace{ "communication::instance::ping"};
         
         return communication::ipc::call( id, common::message::server::ping::Request{ process::handle()}).process;
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
                        lookup::Directive directive)
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

            template< lookup::Directive directive>
            basic_connector< directive>::basic_connector( instance::Identity identity)
               : m_identity{ identity}
            {
               log::line( log, "instance created - identity: ", m_identity);
               log::line( verbose::log, "connector: ", *this);
            }

            template< lookup::Directive directive>
            const process::Handle& basic_connector< directive>::process() 
            { 
               if( ! m_process)
                  (void)connect();

               return m_process;
            }


            template< lookup::Directive directive>
            bool basic_connector< directive>::connect()
            {
               Trace trace{ "communication::instance::outbound::Connector::connect"};

               reset( local::fetch( m_identity, directive));
               log::line( verbose::log, "connector: ", *this);

               return predicate::boolean( m_process);
            }

            template< lookup::Directive directive>
            void basic_connector< directive>::clear()
            {
               Trace trace{ "communication::instance::outbound::Connector::clear"};

               environment::variable::unset( m_identity.environment);
               m_connector.clear();
            }

            template struct basic_connector< lookup::Directive::direct>;
            template struct basic_connector< lookup::Directive::wait>;
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

                  static_assert( device::outbound::is_optional< detail::optional::Device>);
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


               bool Connector::connect()
               {
                  Trace trace{ "communication::instance::outbound::domain::manager::Connector::connect"};

                  reset( local::connect( [](){ return common::domain::singleton::read().process;}));
                  log::line( verbose::log, "connector: ", *this);

                  return predicate::boolean( m_process);
               }

               const process::Handle& Connector::process()
               {
                  if( ! m_process)
                     (void)connect();
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
                  bool Connector::connect()
                  {
                     Trace trace{ "communication::instance::outbound::domain::manager::optional::Connector::connect"};

                     reset( local::connect( [](){
                        return common::domain::singleton::read().process;
                     }));

                     log::line( verbose::log, "connector: ", *this);

                     return predicate::boolean( m_process);
                  }

                  const process::Handle& Connector::process()
                  {
                     if( ! m_process)
                        (void)connect();
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