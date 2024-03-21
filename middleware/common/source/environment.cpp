//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/environment.h"
#include "common/file.h"
#include "common/algorithm.h"
#include "common/range/adapter.h"
#include "common/log.h"
#include "common/string.h"
#include "common/string/view.h"
#include "common/string/compose.h"

#include "common/code/convert.h"
#include "common/code/system.h"


#include <memory>
#include <iomanip>
#include <cstdlib>

#ifdef __APPLE__
   #include <crt_externs.h>
#else
   #include <unistd.h>
#endif

namespace casual
{
   namespace common::environment
   {
      namespace local
      {
         namespace
         {
            auto system()
            {
               #ifdef __APPLE__
                  return *::_NSGetEnviron();
               #else
                  return ::environ;
               #endif
            }

            struct Coordinator
            {
               using lock_type = std::lock_guard< std::mutex>;

               static const Coordinator& instance()
               {
                  static const Coordinator singleton{};
                  return singleton;
               }

               bool exists( std::string_view name) const
               {
                  lock_type lock { m_mutex};

                  if( algorithm::find_if( m_prioritized, environment::variable::predicate::is_name( name)))
                     return true;

                  return getenv( name.data()) != nullptr;
               }

               std::optional< std::string> get( std::string_view name) const
               {
                  lock_type lock { m_mutex};

                  // We need to return by value and copy the variable while
                  // we have the lock

                  if( auto found = algorithm::find_if( m_prioritized, environment::variable::predicate::is_name( name)))
                     return std::string{ found->value()};

                  if( auto result = getenv( name.data()))
                     return result;

                  return {};
               }

               void set( std::string_view name, std::string_view value) const
               {
                  lock_type lock { m_mutex};

                  if( auto found = algorithm::find_if( m_prioritized, environment::variable::predicate::is_name( name)))
                     *found = environment::Variable{ string::compose( name, '=', value)};
                  else
                     m_prioritized.emplace_back( string::compose( name, '=', value));
               }

               void unset( std::string_view name) const
               {
                  lock_type lock { m_mutex};

                  if( auto found = algorithm::find_if( m_prioritized, environment::variable::predicate::is_name( name)))
                     m_prioritized.erase( std::begin( found));

                  if( ::unsetenv( name.data()) == -1)
                     code::raise::error( code::convert::to::casual( code::system::last::error()), "environment::unset");
               }

               std::vector< environment::Variable> system() const
               {
                  std::vector< environment::Variable> result;

                  // take lock
                  std::lock_guard< std::mutex> lock{ m_mutex};

                  auto system = local::system();

                  while( *system)
                     result.emplace_back( *system++);

                  return result;
               }

               //! @return the total set of environment variables `prioritized U system`
               std::vector< environment::Variable> current() const
               {
                  auto system = Coordinator::system();

                  // take lock
                  std::lock_guard< std::mutex> lock{ m_mutex};

                  auto result = m_prioritized;

                  algorithm::append_unique( system, result, environment::variable::predicate::equal_name());

                  return result;
               }

               std::mutex& mutex() const
               {
                  return m_mutex;
               }

            private:

               mutable std::mutex m_mutex;
               mutable std::vector< environment::Variable> m_prioritized;
            };

            auto& coordinator() { return Coordinator::instance();}

         } // <unnamed>
      } // local

      namespace variable
      {
         std::mutex& mutex()
         {
            return local::coordinator().mutex();
         }

         bool exists( std::string_view name)
         {
            return local::coordinator().exists( name);
         }

         std::vector< environment::Variable> system()
         {
            return local::coordinator().system();
         }

         std::vector< environment::Variable> current()
         {
            return local::coordinator().current();
         }

         std::optional< std::string> get( std::string_view name)
         {
            // make sure we got null terminator
            assert( *std::end( name) == '\0');

            return local::coordinator().get( name);
         }

         void set( std::string_view name, std::string_view value)
         {
            local::coordinator().set( name, value);
         }

         std::optional< std::string> consume( std::string_view name)
         {
            if( auto result = get( name))
            {
               unset( name);
               return result;
            }
            return {};
         }

         void unset( std::string_view name)
         {
            local::coordinator().unset( name);
         }

         namespace detail
         {
            template<>
            bool invoke_string_deserialize< bool>( std::string value)
            {
               // I'm not sure if this is a good thing, but I think
               // we rely on boolean strings

               if( value == "true") 
                  return true; 
               else if( value == "false") 
                  return false; 
               else 
                  return value != "0";
            }
            
            
         } // detail

      } // variable

      namespace local
      {
         namespace
         {
            namespace path
            {
               namespace detail
               {
                  constexpr std::string_view casual = ".casual";
               } // detail

               auto domain()
               {
                  return  variable::get< std::filesystem::path>( variable::name::directory::domain).value_or( "./");
               }

               auto transient()
               {
                  return variable::get< std::filesystem::path>( variable::name::directory::transient).value_or( environment::directory::temporary() / detail::casual);
               }

               auto persistent()
               {
                  return variable::get< std::filesystem::path>( variable::name::directory::persistent).value_or( domain() / detail::casual);
               }

               auto install()
               {
                     // TODO: Use some more platform independent solution
                  return variable::get< std::filesystem::path>( variable::name::directory::install).value_or( "/opt/casual");
               }

               auto log()
               {
                  return variable::get< std::filesystem::path>( variable::name::log::path).value_or( domain() / "casual.log");
               }

               auto ipc()
               {
                  return variable::get< std::filesystem::path>( variable::name::directory::ipc).value_or( transient() / "ipc");
               }

               auto queue()
               {
                  return variable::get< std::filesystem::path>( variable::name::directory::queue).value_or( persistent() / "queue");
               }

               auto transaction()
               {
                  return variable::get< std::filesystem::path>( variable::name::directory::transaction).value_or( persistent() / "transaction");
               }

               auto singleton()
               {
                  return domain() / detail::casual / "singleton";
               }
               
            } // path


            //! holds "all" paths based on environment.
            //! main purpose is to be able to reset when running
            //! unittests
            struct Paths 
            {
               std::filesystem::path domain = path::domain();
               std::filesystem::path install = path::install();
               std::filesystem::path log = path::log();
               std::filesystem::path ipc = path::ipc();
               std::filesystem::path queue = path::queue();
               std::filesystem::path transaction = path::transaction();
               std::filesystem::path singleton = path::singleton();
               
               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( domain);
                  CASUAL_SERIALIZE( install);
                  CASUAL_SERIALIZE( log);
                  CASUAL_SERIALIZE( ipc);
                  CASUAL_SERIALIZE( queue);
                  CASUAL_SERIALIZE( transaction);
                  CASUAL_SERIALIZE( singleton);
               )
            };

            
            namespace global
            {
               Paths paths;
            } // global

         } // <unnamed>
      } // local


      namespace directory
      {
         const std::filesystem::path& domain()
         {
            return local::global::paths.domain;
         }

         std::filesystem::path temporary()
         {
            return std::filesystem::temp_directory_path();
         }

         const std::filesystem::path& install()
         {
            return local::global::paths.install;
         }

         const std::filesystem::path& ipc()
         {
               return local::global::paths.ipc;
         }

         const std::filesystem::path& queue()
         {
               return local::global::paths.queue;
         }

         const std::filesystem::path& transaction()
         {
               return local::global::paths.transaction;
         }
      } // directory

      namespace log
      {
         const std::filesystem::path& path()
         {
            return local::global::paths.log;
         }
      } // log

      namespace domain
      {
         namespace singleton
         {
            const std::filesystem::path& file()
            { 
               return local::global::paths.singleton;
            }
         } // singleton
      } // domain


      void reset()
      {
         local::global::paths = local::Paths{};
      }

   } // common::environment
} // casual

