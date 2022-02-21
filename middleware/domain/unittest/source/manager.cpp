//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/unittest/manager.h"
#include "domain/manager/task.h"
#include "domain/manager/admin/server.h"

#include "common/environment.h"
#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/event/listen.h"
#include "common/execute.h"
#include "common/message/event.h"
#include "common/message/handle.h"
#include "common/serialize/binary.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include "common/unittest.h"
#include "common/unittest/file.h"

namespace casual
{
   using namespace common;

   namespace domain::unittest
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

               auto arguments( const std::vector< file::scoped::Path>& files, const strong::correlation::id& id)
               {
                  std::vector< std::string> result{ 
                     "--bare", "true",
                     "--event-ipc", common::string::compose( common::communication::ipc::inbound::ipc()),
                     "--event-id", common::string::compose( id),
                     "--configuration-files"
                  };
                  algorithm::append( 
                     algorithm::transform( 
                        files, 
                        []( const auto& file) 
                        { return file.string();}), 
                     result);

                  return result;
               }

            } // configuration

            namespace repository
            {
               auto root()
               {
                  return environment::variable::get( "CASUAL_MAKE_SOURCE_ROOT");
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


            void shutdown( const process::Handle& manager)
            {
               log::Trace trace{ "domain::unittest::local::shutdown", verbose::log};
               log::line( verbose::log, "manager: ", manager);

               signal::thread::scope::Block blocked_signals{ { code::signal::child}};

               auto gracefully = []( auto& manager)
               {
                  auto handler = []( auto& tasks)
                  {
                     return message::dispatch::handler( communication::ipc::inbound::device(),
                        [&tasks]( message::event::Task& event)
                        {
                           if( event.done())
                              algorithm::container::trim( tasks, algorithm::remove( tasks, event.correlation));
                        },
                        []( message::event::Error& event)
                        {
                           log::line( log::category::error, "event: ", event);
                        }
                     );
                  };

                  auto tasks = communication::ipc::call(
                      manager.ipc, common::message::domain::manager::shutdown::Request{ process::handle()}).tasks;

                  auto condition = common::message::dispatch::condition::compose(
                     common::event::condition::done( [&tasks]()
                     { 
                        return tasks.empty();
                     })
                  );

                  // listen for events
                  common::message::dispatch::relaxed::pump( 
                     condition,
                     handler( tasks), 
                     communication::ipc::inbound::device());

                  process::wait( manager.pid);
               };

               if( manager)
                  gracefully( manager);
               else if( manager.pid)
               {
                  process::terminate( manager.pid);
                  process::wait( manager.pid);
               }
            }


            struct Manager
            {
               Manager() = default;
               Manager( const std::string& path, std::vector< std::string> arguments)
                  : process{ path, arguments} 
               {}

               ~Manager()
               {
                  exception::guard( [&]()
                  {
                     if( process)
                        local::shutdown( process);
                     
                     process.clear();
                  });
               }

               Manager( Manager&& other) noexcept = default;
               Manager& operator = ( Manager&& other) noexcept = default;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( process);
               )

               common::Process process;
            };

         } // <unnamed>
      } // local

      struct Manager::Implementation
      {
         Implementation( std::vector< std::string_view> configuration, std::function< void( const std::string&)> callback = nullptr)
            : environment( std::move( callback)),
            files( local::configuration::files( configuration))
         {
            log::Trace trace{ "domain::unittest::Manager::Implementation", verbose::log};

            common::domain::identity( {});

            local::instance::devices::reset();

            auto unsubscribe_scope = execute::scope( [&]()
            {
               if( ! manager.process.ipc)
                  return;

               communication::device::blocking::optional::send( 
                  manager.process.ipc, common::message::event::subscription::End{ process::handle()});
            });
            
            auto condition = []( auto& tasks)
            { 
               return message::dispatch::condition::compose( 
                  event::condition::done( [&tasks]()
                  {
                     // we're done waiting when we got the ipc of domain-manager
                     return tasks.empty();
                  }));
            };


            auto handler = []( auto& state, auto& tasks)
            {
               return common::message::dispatch::handler( communication::ipc::inbound::device(),
                  common::message::handle::discard< message::event::process::Spawn>(),
                  common::message::handle::discard< message::event::process::Exit>(),
                  common::message::handle::discard< message::event::sub::Task>(),
                  [&]( const manager::task::message::domain::Information& event)
                  {
                     log::line( log::debug, "event: ", event);
                     state.domain = event.domain;
                     common::domain::identity( event.domain);
                     state.manager.process.handle( event.process);

                     // Set environment variable to make it easier for other processes to
                     // reach domain-manager (should work any way...)
                     common::environment::variable::process::set(
                        common::environment::variable::name::ipc::domain::manager,
                        state.manager.process);
                  },
                  [&tasks]( const message::event::Task& event)
                  {
                     log::line( log::debug, "event: ", event);

                     if( event.done())
                        algorithm::container::trim( tasks, algorithm::remove( tasks, event.correlation));
                  },
                  []( const message::event::Error& event)
                  {
                     log::line( log::debug, "event: ", event);

                     if( event.severity == decltype( event.severity)::fatal)
                        code::raise::error( code::casual::shutdown, "fatal error: ", event);
                  });
            };

            auto tasks = std::vector< strong::correlation::id>{ strong::correlation::id::emplace( uuid::make())};

            // spawn the domain-manager
            manager = local::Manager{ local::repository::root() + "/middleware/domain/bin/casual-domain-manager",
               local::configuration::arguments( files, tasks.front())};

            common::message::dispatch::relaxed::pump( 
               condition( tasks), 
               handler( *this, tasks), 
               communication::ipc::inbound::device());

            log::line( verbose::log, "domain-manager booted: ", manager);
            
         }

         Implementation() : Implementation( { R"(
domain:
   name: default-domain
)"})
         {}


         ~Implementation()
         {}

         void activate()
         {
            log::Trace trace{ "domain::unittest::Manager::Implementation::activate", verbose::log};

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
               CASUAL_SERIALIZE( home);
            )

         } environment;


         std::vector< common::file::scoped::Path> files;
         local::Manager manager;
         common::domain::Identity domain;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( files);
            CASUAL_SERIALIZE( manager);
            CASUAL_SERIALIZE( domain);
         )

      };

      Manager::Manager( std::vector< std::string_view> configuration)
         : m_implementation( configuration) {}

      Manager::Manager( std::vector< std::string_view> configuration, std::function< void( const std::string&)> callback)
         : m_implementation( configuration, std::move( callback)) {}

      Manager::Manager() {}

      Manager::~Manager()
      {
         log::Trace trace{ "domain::unittest::Manager::~Manager", verbose::log};
         log::line( verbose::log, "this: ", *this);
      }

      Manager::Manager( Manager&&) noexcept = default;
      Manager& Manager::operator = ( Manager&&) noexcept = default;

      const common::process::Handle& Manager::handle() const noexcept
      {
         return m_implementation->manager.process;
      }

      void Manager::activate()
      {
         m_implementation->activate();
      }

      std::ostream& operator << ( std::ostream& out, const Manager& value)
      {
         if( value.m_implementation)
            return common::stream::write( out, *value.m_implementation);
         return out << "nil";
      }

   } // domain::unittest
} // casual