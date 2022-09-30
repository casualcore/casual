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
               namespace path
               {
                  namespace detail
                  {
                     constexpr std::string_view casual = ".casual";


                     template< typename O>
                     auto value( std::string_view name, O&& optional) -> std::filesystem::path
                     {
                        if( variable::exists( name))
                           return variable::get( name);

                        return optional();
                     }
                  } // detail

                  auto domain() -> std::filesystem::path
                  {
                     return detail::value( variable::name::directory::domain, [](){ return "./";});
                  }

                  auto transient() -> std::filesystem::path
                  {
                     return detail::value( variable::name::directory::transient, [](){ return environment::directory::temporary() / detail::casual;});
                  }

                  auto persistent() -> std::filesystem::path
                  {
                     return detail::value( variable::name::directory::persistent, [](){ return domain() / detail::casual;});
                  }

                  auto install() -> std::filesystem::path
                  {
                      // TODO: Use some more platform independent solution
                     return detail::value( variable::name::directory::install, [](){ return "/opt/casual";});
                  }

                  auto log() -> std::filesystem::path
                  {
                     return detail::value( variable::name::log::path, [](){ return domain() / "casual.log";});
                  }

                  auto ipc() -> std::filesystem::path
                  {
                     return detail::value( variable::name::directory::ipc, [](){ return transient() / "ipc";});
                  }

                  auto queue() -> std::filesystem::path
                  {
                     return detail::value( variable::name::directory::queue, [](){ return persistent() / "queue";});
                  }

                  auto transaction() -> std::filesystem::path
                  {
                     return detail::value( variable::name::directory::transaction, [](){ return persistent() / "transaction";});
                  }

                  auto singleton() -> std::filesystem::path
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
      } // environment
   } // common
} // casual

