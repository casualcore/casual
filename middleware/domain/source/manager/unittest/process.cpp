//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/manager/unittest/process.h"

#include "common/environment.h"
#include "common/communication/ipc.h"
#include "common/event/listen.h"
#include "common/execute.h"
#include "common/exception/casual.h"
#include "common/message/event.h"
#include "common/message/handle.h"

#include "common/unittest.h"
#include "common/unittest/file.h"

namespace casual
{
   using namespace common;

   namespace domain
   {
      namespace manager
      {
         namespace unittest
         {
            namespace local
            {
               namespace
               {
                  namespace configuration
                  {

                     std::vector< file::scoped::Path> files( const std::vector< std::string>& configuration)
                     {
                        return algorithm::transform( configuration, []( const std::string& c){
                           return file::scoped::Path{ common::unittest::file::temporary::content( ".yaml", c)};
                        });
                     }

                     auto arguments( const std::vector< file::scoped::Path>& files)
                     {
                        std::vector< std::string> result{ 
                           "--bare", "true",
                           "--event-ipc", common::string::compose( common::communication::ipc::inbound::ipc()),
                           "--configuration-files"
                        };
                        algorithm::append( files, result);

                        return result;
                     }

                  } // configuration

                  namespace repository
                  {
                     auto root()
                     {
                        return environment::variable::get( "CASUAL_REPOSITORY_ROOT");
                     }
                  } // repository
               } // <unnamed>
            } // local

            struct Process::Implementation
            {
               Implementation( const std::vector< std::string>& configuration, std::function< void( const std::string&)> callback = nullptr)
                  : environment( std::move( callback)),
                  files( local::configuration::files( configuration)),
                  process{ common::environment::directory::casual() + "/bin/casual-domain-manager", local::configuration::arguments( files)}
               {
                  log::Trace trace{ "domain::manager::unittest::Process::Implementation", verbose::log};

                  // Make sure we unregister the event subscription
                  auto unsubscribe = common::execute::scope( [](){
                     common::event::unsubscribe( common::process::handle(), { common::message::Type::event_domain_error});
                  });

                  // Wait for the domain to boot
                  process.handle( process::wait());

                  log::line( verbose::log, "domain-manager booted: ", process);
                  
                  // Set environment variable to make it easier for other processes to
                  // reach domain-manager (should work any way...)
                  common::environment::variable::process::set(
                     common::environment::variable::name::ipc::domain::manager(),
                     process.handle());
                  
               }

               Implementation() : Implementation( { R"(
domain:
   name: default-domain
               )"})
               {}
               
               struct Environment
               {
                  Environment( std::function< void( const std::string&)> callback)
                     : callback{ std::move( callback)}
                  {
                     activate();
                  }

                  void activate()
                  {
                     environment::variable::set( "CASUAL_HOME", local::repository::root() + "/test/home");
                     environment::variable::set( "CASUAL_DOMAIN_HOME", home);
                     
                     if( callback)
                        callback( home);

                     // reset all (hopefolly) environment based 'values' 
                     environment::reset();
                  }
   
                  //! domain root directory
                  common::unittest::directory::temporary::Scoped home;
                  std::function< void( const std::string&)> callback;

                  CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
                  {
                     CASUAL_SERIALIZE( home);
                  })

               } environment;

               std::vector< common::file::scoped::Path> files;
               common::Process process;

               CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
               {
                  CASUAL_SERIALIZE( files);
                  CASUAL_SERIALIZE( process);
               })

            };

            Process::Process( const std::vector< std::string>& configuration)
               : m_implementation( configuration) {}

            Process::Process( const std::vector< std::string>& configuration, std::function< void( const std::string&)> callback)
               : m_implementation( configuration, std::move( callback)) {}

            Process::Process() {}

            Process::~Process() = default;

            Process::Process( Process&&) = default;
            Process& Process::operator = ( Process&&) = default;

            const common::process::Handle& Process::handle() const noexcept
            {
               return m_implementation->process.handle();
            }

            void Process::activate()
            {
               m_implementation->environment.activate();
            }

            std::ostream& operator << ( std::ostream& out, const Process& value)
            {
               return common::stream::write( out, *value.m_implementation);
            }
             

            namespace process
            {  
               common::process::Handle wait( common::communication::ipc::inbound::Device& device)
               {
                  common::Trace trace{ "domain::manager::unittest::process::wait"};

                  auto handler = device.handler(
                     []( const message::event::domain::boot::Begin& event)
                     {
                        log::line( log::debug, "event: ", event);
                        common::domain::identity( event.domain);
                     },
                     []( const message::event::domain::boot::End& event)
                     {
                        log::line( log::debug, "event: ", event);
                        throw event.process;
                     },
                     []( const message::event::domain::Error& error)
                     {
                        if( error.severity == message::event::domain::Error::Severity::fatal)
                        {
                           throw exception::casual::Shutdown{ string::compose( "fatal error: ", error)};
                        }
                     },
                     common::message::handle::discard< common::message::event::domain::Group>(),
                     common::message::handle::discard< common::message::event::domain::shutdown::Begin>(),
                     common::message::handle::discard< common::message::event::domain::shutdown::End>(),
                     common::message::handle::discard< common::message::event::domain::server::Connect>(),
                     common::message::handle::discard< common::message::event::domain::task::Begin>(),
                     common::message::handle::discard< common::message::event::domain::task::End>(),
                     common::message::handle::discard< common::message::event::process::Spawn>(),
                     common::message::handle::discard< common::message::event::process::Exit>()
                  );

                  try
                  {
                     message::dispatch::blocking::pump( handler, device);
                  }
                  catch( const common::process::Handle& process)
                  {
                     log::line( verbose::log, "domain manager booted: ", process);
                     return process;
                  }
                  return {};
               }

               common::process::Handle wait()
               {
                  return wait( communication::ipc::inbound::device());
               }
            } // process

         } // unittest
      } // manager
   } // domain
} // casual