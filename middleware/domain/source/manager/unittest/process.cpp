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

#include "common/mockup/file.h"
#include "common/unittest.h"

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
                           return file::scoped::Path{ mockup::file::temporary::content( ".yaml", c)};
                        });
                     }

                     std::string names( const std::vector< file::scoped::Path>& files)
                     {
                        return string::join( files, " ");
                     }

                  } // configuration

                  namespace repository
                  {
                     auto root = environment::variable::get( "CASUAL_REPOSITORY_ROOT");
                  } // repository
               } // <unnamed>
            } // local

            struct Process::Implementation
            {
               Implementation( const std::vector< std::string>& configuration)
                  : files( local::configuration::files( configuration)),
                  process{ "${CASUAL_HOME}/bin/casual-domain-manager", {
                     "--event-ipc", common::string::compose( common::communication::ipc::inbound::ipc()),
                     "--configuration-files", local::configuration::names( files),
                     "--bare", "true" }}
               {

                  // Make sure we unregister the event subscription
                  auto unsubscribe = common::execute::scope( [](){
                     common::event::unsubscribe( common::process::handle(), { common::message::Type::event_domain_error});
                  });

                  // Wait for the domain to boot
                  process.handle( common::unittest::domain::manager::wait( common::communication::ipc::inbound::device()));

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
                  Environment()
                  {
                     environment::variable::set( "CASUAL_HOME", local::repository::root + "/test/home");
                     environment::variable::set( "CASUAL_DOMAIN_HOME", home);

                     environment::reset();

                     if( file::exists( environment::domain::singleton::file()))
                     {
                        file::remove( environment::domain::singleton::file());
                     }
                  }
                     
                  //! domain root directory
                  common::mockup::directory::temporary::Scoped home;
               } environment;

               std::vector< common::file::scoped::Path> files;
               common::Process process;

            };

            Process::Process( const std::vector< std::string>& configuration)
               : m_implementation( configuration) {}

            Process::Process() {}

            Process::~Process() = default;

            const common::process::Handle& Process::handle() const noexcept
            {
               return m_implementation->process.handle();
            }

            std::ostream& operator << ( std::ostream& out, const Process& value)
            {
               return out << "{ home: " << value.m_implementation->environment.home
                  << ", files: " << value.m_implementation->files
                  << ", process: " << value.m_implementation->process 
                  << '}';
            }
             
         } // unittest
      } // manager
   } // domain
} // casual