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
   namespace common
   {
      namespace environment
      {
         namespace local
         {
            namespace
            {
               namespace native
               {
                  struct Variable
                  {
                     using lock_type = std::lock_guard< std::mutex>;

                     static const Variable& instance()
                     {
                        static const Variable singleton{};
                        return singleton;
                     }

                     bool exists( std::string_view name) const
                     {
                        lock_type lock { m_mutex};

                        return getenv( name.data()) != nullptr;
                     }

                     std::string get( std::string_view name) const
                     {
                        lock_type lock { m_mutex};

                        auto result = getenv( name.data());

                        // We need to return by value and copy the variable while
                        // we have the lock
                        if( result)
                           return result;

                        return {};
                     }

                     void set( std::string_view name, std::string_view value) const
                     {
                        lock_type lock { m_mutex};

                        if( setenv( name.data(), value.data(), 1) == -1)
                           code::raise::error( code::convert::to::casual( code::system::last::error()), "environment::set");
                     }

                     void unset( std::string_view name) const
                     {
                        lock_type lock { m_mutex};

                        if( ::unsetenv( name.data()) == -1)
                           code::raise::error( code::convert::to::casual( code::system::last::error()), "environment::unset");
                     }

                     std::mutex& mutex() const
                     {
                        return m_mutex;
                     }

                  private:

                     mutable std::mutex m_mutex;
                  };

                  auto environment()
                  {
                     #ifdef __APPLE__
                        return *::_NSGetEnviron();
                     #else
                        return ::environ;
                     #endif
                  }

               } // native
            } // <unnamed>
         } // local

         namespace variable
         {
            namespace detail
            {
               void set( std::string_view name, std::string_view value)
               {
                  local::native::Variable::instance().set( name, value);
               }

            } // detail
            std::mutex& mutex()
            {
               return local::native::Variable::instance().mutex();
            }

            bool exists( std::string_view name)
            {
               return local::native::Variable::instance().exists( name);
            }

            std::string get( std::string_view name)
            {
               // make sure we got null terminator
               assert( *std::end( name) == '\0');

               return local::native::Variable::instance().get( name);
            }

            void unset( std::string_view name)
            {
               local::native::Variable::instance().unset( name);
            }

            namespace native
            {
               std::vector< environment::Variable> current()
               {
                  std::vector< environment::Variable> result;

                  // take lock
                  std::lock_guard< std::mutex> lock{ variable::mutex()};

                  auto variable = local::native::environment();

                  while( *variable)
                  {
                     result.emplace_back( *variable);
                     ++variable;
                  }
                  return result;
               }
            } // native


            namespace process
            {
               common::process::Handle get( std::string_view variable)
               {
                  auto value = common::environment::variable::get( variable);

                  common::process::Handle result;
                  {
                     auto split = algorithm::split(value, '|');

                     auto pid = std::get < 0 > ( split);
                     if( common::string::integer( pid))
                        result.pid = strong::process::id{ std::stoi( std::string( std::begin( pid), std::end( pid)))};

                     auto ipc = std::get< 1>( split);
                     if( ! ipc.empty())
                        result.ipc = strong::ipc::id{ Uuid( string::view::make( ipc))};
                  }

                  return result;
               }

               void set( std::string_view variable, const common::process::Handle& process)
               {
                  variable::detail::set( variable, string::compose( process.pid, '|', process.ipc));
               }

            } // process
         } // variable

         namespace local
         {
            namespace
            {
               //! holds "all" paths based on enviornment.
               //! main purpose is to be able to reset when running
               //! unittests
               struct Paths 
               {
                  std::filesystem::path domain = create_path( get_domain());
                  std::filesystem::path detail = create_path( get_domain() / ".casual");
                  std::filesystem::path tmp = environment::directory::temporary();
                  
                  std::filesystem::path casual = []() -> std::filesystem::path
                  {
                     if( variable::exists( variable::name::home))
                        return variable::get( variable::name::home);

                     // TODO: Use some more platform independant solution
                     return "/opt/casual";
                  }();

                  std::filesystem::path log = []() -> std::filesystem::path
                  {
                     auto file = []( ) -> std::filesystem::path
                     {
                        if( variable::exists( variable::name::log::path))
                           return variable::get( variable::name::log::path);

                        return get_domain() / "casual.log";
                     }();

                     common::directory::create( file.parent_path());
                     return file;
                  }();

                  std::filesystem::path ipc = []() 
                  {
                     auto get = []() -> std::filesystem::path
                     {
                        if( variable::exists( variable::name::ipc::directory))
                           return variable::get( variable::name::ipc::directory);

                        return environment::directory::temporary() / ".casual" / "ipc";
                     };
                     return create_path( get());
                  }();

                  std::filesystem::path singleton = get_domain() / ".casual" / "singleton";

                  
                  CASUAL_LOG_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( domain);
                     CASUAL_SERIALIZE( detail);
                     CASUAL_SERIALIZE( tmp);
                     CASUAL_SERIALIZE( casual);
                     CASUAL_SERIALIZE( log);
                     CASUAL_SERIALIZE( ipc);
                     CASUAL_SERIALIZE( singleton);
                  })

               private:

                  static std::filesystem::path get_domain()
                  {
                     if( variable::exists( variable::name::domain::home))
                        return { variable::get( variable::name::domain::home)};

                     return { "./"};
                  }

                  static std::filesystem::path create_path( std::filesystem::path path)
                  {
                     common::directory::create( path);
                     return path;
                  }
               };

               
               namespace global
               {
                  Paths paths;
               } // global
               

               Paths& paths() 
               {
                  return global::paths;
               }
            } // <unnamed>
         } // local


         namespace directory
         {
            const std::filesystem::path& domain()
            {
               return local::paths().domain;
            }

            const std::filesystem::path& temporary()
            {
               static const std::filesystem::path singleton{ common::directory::temporary()};
               return singleton;               
            }

            const std::filesystem::path& casual()
            {
               return local::paths().casual;
            }
         } // directory

         namespace log
         {
            const std::filesystem::path& path()
            {
               return local::paths().log;
            }
         } // log

         namespace ipc
         {
            const std::filesystem::path& directory()
            {
                return local::paths().ipc;
            }
         } // transient

         namespace domain
         {
            namespace singleton
            {
               const std::filesystem::path& file()
               { 
                  return local::paths().singleton;
               }
            } // singleton
         } // domain


         void reset()
         {
            local::paths() = local::Paths{};

            common::log::line( common::verbose::log, "paths: ", local::paths());
         }
      } // environment
   } // common
} // casual

