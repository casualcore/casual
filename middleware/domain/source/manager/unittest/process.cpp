//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/manager/unittest/process.h"
#include "domain/manager/task.h"

#include "common/environment.h"
#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/event/listen.h"
#include "common/execute.h"
#include "common/message/event.h"
#include "common/message/handle.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

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

                     std::vector< file::scoped::Path> files( std::vector< std::string_view> configuration)
                     {
                        return algorithm::transform( configuration, []( auto& c){
                           return file::scoped::Path{ common::unittest::file::temporary::content( ".yaml", c)};
                        });
                     }

                     auto arguments( const std::vector< file::scoped::Path>& files, const common::Uuid& id)
                     {
                        std::vector< std::string> result{ 
                           "--bare", "true",
                           "--event-ipc", common::string::compose( common::communication::ipc::inbound::ipc()),
                           "--event-id", common::string::compose( id),
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
                  
                  namespace instance::devices
                  {
                     void reset()
                     {
                        // reset domain instance, if any.
                        exception::guard( [](){ communication::instance::outbound::domain::manager::device().connector().clear();});
                        exception::guard( [](){ communication::instance::outbound::domain::manager::optional::device().connector().clear();});

                        // reset service instance, if any.
                        exception::guard( [](){ communication::instance::outbound::service::manager::device().connector().clear();});
                     }

                  } // instance::devices

               } // <unnamed>
            } // local

            struct Process::Implementation
            {
               Implementation( std::vector< std::string_view> configuration, std::function< void( const std::string&)> callback = nullptr)
                  : environment( std::move( callback)),
                  files( local::configuration::files( configuration))
               {
                  log::Trace trace{ "domain::manager::unittest::Process::Implementation", verbose::log};

                  common::domain::identity( {});

                  local::instance::devices::reset();

                  auto tasks = std::vector< common::Uuid>{ uuid::make()};

                  auto condition = event::condition::compose( 
                     event::condition::prelude( [&]()
                     {
                        // spawn the domain-manager
                        process = common::Process{ 
                           local::repository::root() + "/middleware/domain/bin/casual-domain-manager", 
                           local::configuration::arguments( files, tasks.front())};
                     }),
                     event::condition::done( [&tasks]()
                     {
                        // we're done waiting when we got the ipc of domain-manager
                        return tasks.empty();
                     })
                  );

                  // let's boot and listen to events
                  event::only::unsubscribe::listen( condition,
                     [&]( const manager::task::message::domain::Information& event)
                     {
                        log::line( log::debug, "event: ", event);
                        domain = event.domain;
                        common::domain::identity( domain);
                        process.handle( event.process);
                     },
                     [&tasks]( const message::event::Task& event)
                     {
                        log::line( log::debug, "event: ", event);

                        if( event.done())
                           algorithm::trim( tasks, algorithm::remove( tasks, event.correlation));
                     },
                     []( const message::event::Error& event)
                     {
                        log::line( log::debug, "event: ", event);

                        if( event.severity == decltype( event.severity)::fatal)
                           code::raise::error( code::casual::shutdown, "fatal error: ", event);
                     }
                  );

                  log::line( verbose::log, "domain-manager booted: ", process);
                  
                  // Set environment variable to make it easier for other processes to
                  // reach domain-manager (should work any way...)
                  common::environment::variable::process::set(
                     common::environment::variable::name::ipc::domain::manager,
                     process.handle());
                  
               }

               Implementation() : Implementation( { R"(
domain:
   name: default-domain
               )"})
               {}

               void activate()
               {
                  log::Trace trace{ "domain::manager::unittest::Process::Implementation::activate", verbose::log};

                  common::domain::identity( domain);

                  environment.activate();

                  local::instance::devices::reset();
               }
               
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

                  CASUAL_LOG_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( home);
                  })

               } environment;


               std::vector< common::file::scoped::Path> files;
               common::Process process;
               common::domain::Identity domain;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( files);
                  CASUAL_SERIALIZE( process);
                  CASUAL_SERIALIZE( domain);
               )

            };

            Process::Process( std::vector< std::string_view> configuration)
               : m_implementation( configuration) {}

            Process::Process( std::vector< std::string_view> configuration, std::function< void( const std::string&)> callback)
               : m_implementation( configuration, std::move( callback)) {}

            Process::Process() {}

            Process::~Process()
            {
               log::Trace trace{ "domain::manager::unittest::Process::~Process", verbose::log};
               log::line( verbose::log, "this: ", *this);
            }

            Process::Process( Process&&) noexcept = default;
            Process& Process::operator = ( Process&&) noexcept = default;

            const common::process::Handle& Process::handle() const noexcept
            {
               return m_implementation->process.handle();
            }

            void Process::activate()
            {
               m_implementation->activate();
            }

            std::ostream& operator << ( std::ostream& out, const Process& value)
            {
               if( value.m_implementation)
                  return common::stream::write( out, *value.m_implementation);
               return out << "nil";
            }

         } // unittest
      } // manager
   } // domain
} // casual