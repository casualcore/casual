//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/process.h"

#include "common/exception/handle.h"
#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/code/system.h"
#include "common/code/convert.h"

#include "common/file.h"
#include "common/log.h"
#include "common/signal.h"
#include "common/signal/timer.h"
#include "common/string.h"
#include "common/environment.h"
#include "common/environment/normalize.h"
#include "common/uuid.h"
#include "common/result.h"

#include "common/message/domain.h"
#include "common/communication/ipc.h"
#include "common/flag.h"
#include "common/chronology.h"


#include <algorithm>
#include <functional>
#include <fstream>
#include <system_error>


#include <unistd.h>
#include <sys/wait.h>
#include <csignal>
//#include <fcntl.h>

#include <spawn.h>


#ifdef __APPLE__
   #include <mach-o/dyld.h>
#else
   #include <unistd.h>
#endif



namespace casual
{
   namespace common
   {
      namespace process
      {
         namespace local
         {
            namespace
            {
               std::string get_process_path()
               {
#ifdef __APPLE__
                  std::uint32_t size = platform::size::max::path;
                  std::vector< char> path( platform::size::max::path);
                  if( ::_NSGetExecutablePath( path.data(), &size) != 0)
                     code::raise::log( code::casual::invalid_path, "failed to get the path to the current executable");

                  if( path.data()) 
                     return path.data();

                  return {};
#else
                  return file::name::link( "/proc/self/exe");
#endif
               }


               namespace global
               {
                  auto pid = strong::process::id{ ::getpid()};
                  
               } // global

               const Handle& handle()
               {
                  static const Handle result{ 
                     global::pid, 
                     communication::ipc::inbound::ipc()};
                  return result;
               }


               namespace global
               {
                  const std::string path = get_process_path();
                  const std::string basename = file::name::base( path);
                  const std::string directory = directory::name::base( path);

               } // global

            } // <unnamed>
         } // local

         const std::string& path()
         {
            return local::global::path;
         }

         // TODO maintenance: could be std::string_view
         const std::string& basename()
         {
            return local::global::basename;
         }

         // TODO maintenance: could be std::string_view
         const std::string& directory()
         {
            return local::global::directory;
         }

         strong::process::id id()
         {
            return local::global::pid;
         }

         const Handle& handle()
         {
            return local::handle();
         }

         void sleep( platform::time::unit time)
         {
            log::line( verbose::log, "process::sleep time: ", time);

            timespec posix_time;
            posix_time.tv_sec = std::chrono::duration_cast< std::chrono::seconds>( time).count();
            posix_time.tv_nsec = std::chrono::duration_cast< std::chrono::nanoseconds>(
                  time - std::chrono::seconds{ posix_time.tv_sec}).count();

            // We check signals before we sleep
            signal::dispatch();

            if( ::nanosleep( &posix_time, nullptr) == -1)
            {
               switch( code::system::last::error())
               {
                  case std::errc::interrupted:
                     signal::dispatch();
                     break;
                  default:
                     code::system::raise();
               }
            }
         }

         namespace pattern
         {
            namespace local
            {
               namespace
               {
                  platform::size::type check_infinity( platform::size::type quantity)
                  {
                     return quantity ==  0 ? std::numeric_limits< platform::size::type>::max() : quantity;
                  }
               } // <unnamed>
            } // local


            Sleep::Pattern::Pattern( platform::time::unit time, platform::size::type quantity)
               : m_time{ time}, m_quantity{ local::check_infinity( quantity)}
            {}

            Sleep::Pattern::Pattern( platform::time::unit time, infinite_quantity)
               : m_time{ time}, m_quantity{ std::numeric_limits< platform::size::type>::max()}
            {}

            bool Sleep::Pattern::done()
            {
               sleep( m_time);

               if( m_quantity == std::numeric_limits< platform::size::type>::max())
               {
                  return false;
               }
               return --m_quantity == 0;
            }

            Sleep::Sleep( std::initializer_list< Pattern> pattern) : m_pattern( std::move( pattern))
            {
               // We reverse the patterns so we can go from the back
               algorithm::reverse( m_pattern);
            }

            bool Sleep::operator () ()
            {
               if( ! m_pattern.empty())
               {
                  if( m_pattern.back().done())
                  {
                     m_pattern.pop_back();
                  }
                  return true;
               }
               return false;
            }

         } // pattern

         namespace local
         {
            namespace
            {
               namespace current
               {
                  namespace copy
                  {
                     // Appends from current process environment, if it's not
                     // already overridden.
                     auto environment( std::vector< environment::Variable> variables)
                     {
                        auto current = environment::variable::native::current();

                        auto equal_name = []( auto& l, auto& r)
                        {
                           return algorithm::equal( l.name(), r.name());
                        };

                        // use the complement of the intersection
                        auto not_overridden = std::get< 1>( algorithm::intersection( current, variables, equal_name));

                        // move append the variables
                        algorithm::move( not_overridden, variables);

                        return variables;
                     }
                  } // copy

               } // current

               namespace C
               {
                  auto string_data = []( auto& v){ return v.data();};

                  template< typename E>
                  std::vector< const char*> environment( const E& environment)
                  {
                     auto result = algorithm::transform( environment, C::string_data);
                     result.push_back( nullptr);

                     return result;
                  }

                  std::vector< const char*> arguments( const std::string& path, const std::vector< std::string>& arguments)
                  {
                     std::vector< const char*> result;

                     // think we must add application-name as first argument...
                     result.push_back( path.c_str());

                     algorithm::transform( arguments, result, C::string_data);

                     // Null end
                     result.push_back( nullptr);

                     return result;
                  }

               } // C

               namespace spawn
               {
                  struct Attributes : traits::uncopyable
                  {
                     Attributes()
                     {
                        check_error( posix_spawnattr_init( &m_attributes), "posix_spawnattr_init");

                        // We try to eliminate signals to propagate to children by it self...
                        // we don't need to set groupid with posix_spawnattr_setpgroup since the default is 0.
                        check_error( posix_spawnattr_setflags( &m_attributes, POSIX_SPAWN_SETPGROUP), "posix_spawnattr_setflags");
                     }

                     ~Attributes()
                     {
                        posix_spawnattr_destroy( &m_attributes);
                     }

                     posix_spawnattr_t* get() { return &m_attributes;}

                  private:

                     void check_error( int result, const char* message)
                     {
                        if( result == 0)
                           return;

                        auto code = static_cast< std::errc>( result);
                        code::raise::error( code::convert::to::casual( code), message);
                     };

                     posix_spawnattr_t m_attributes;
                  };
               } // spawn

            } // <unnamed>
         } // local


         strong::process::id spawn(
            std::string path,
            std::vector< std::string> arguments,
            std::vector< environment::Variable> environment)
         {
            Trace trace{ "process::spawn"};

            // We need to expand environment and arguments
            environment::normalize( environment, environment);
            environment::normalize( path, environment);
            environment::normalize( arguments, environment);
            

            // check if path exist and process has permission to execute it.
            // could still go wrong, since we don't know if the path will actually execute,
            // but we'll probably get rid of most of the errors (due to bad configuration and such)
            if( ! file::permission::execution( path))
               code::raise::log( code::casual::invalid_path, "spawn failed - path: ", path);

            log::line( log::debug, "process::spawn ", path, ' ', arguments);
            log::line( verbose::log, "environment: ", environment);

            // Append current, if not "overridden"
            environment = local::current::copy::environment( std::move( environment));

            auto pid = [&](){
               local::spawn::Attributes attributes;

               // prepare c-style arguments and environment
               auto c_arguments = local::C::arguments( path, arguments);
               auto c_environment = local::C::environment( environment);

               platform::process::native::type native_pid{};

               // make sure we don't block interupt and terminate
               signal::thread::scope::Unblock unblock{ signal::Set{ code::signal::interrupt, code::signal::terminate}};

               auto status = posix_spawnp(
                  &native_pid,
                  path.c_str(),
                  nullptr,
                  attributes.get(),
                  const_cast< char* const*>( c_arguments.data()),
                  const_cast< char* const*>( c_environment.data()));

               if( status != 0)
                  code::raise::log( code::casual::invalid_path, "spawn failed - path: ", path, " system: ", static_cast< std::errc>( status));
            
               return strong::process::id{ native_pid};
            }();

            // We try to minimize the glitch where the spawned process does not
            // get signals for a short period of time. We need to block so we don't
            // get child-terminate signals (or other signals for that matter...)
            signal::thread::scope::Block block;

            process::sleep( 200us);

            log::line( log::debug, "process::spawned pid: ", pid );

            //
            // Try to figure out if the process started correctly..
            //
            /* We can't really do this, since we mess up the semantics for the caller

            {
               auto deaths = lifetime::wait( { pid}, 1us);

               if( ! deaths.empty() && deaths.front().reason != lifetime::Exit::Reason::exited)
               {
                  auto& reason = deaths.front();

                  throw exception::system::invalid::Argument( "spawn failed", CASUAL_NIP( path),
                        exception::make_nip( "arguments", range::make( arguments)),
                        exception::make_nip( "environment", range::make( environment)),
                        CASUAL_NIP( reason));
               }
            }
            */
            // TODO: try something else to detect if the process started correct or not.

            return pid;
         }

         strong::process::id spawn( std::string path, std::vector< std::string> arguments)
         {
            return spawn( std::move( path), std::move( arguments), {});
         }

         int execute( std::string path, std::vector< std::string> arguments)
         {
            return wait( spawn( std::move( path), std::move( arguments)));
         }


         namespace local
         {
            namespace
            {
               lifetime::Exit wait( strong::process::id pid, int flags = WNOHANG)
               {
                  // we block all signals but the _shutdown_
                  signal::thread::scope::Mask block{ signal::set::filled( code::signal::terminate, code::signal::interrupt)};

                  lifetime::Exit exit;

                  auto loop = [&]()
                  {
                     auto result = waitpid( pid.value(), &exit.status, flags);

                     if( result == -1)
                     {
                        switch( auto code = code::system::last::error())
                        {
                           case std::errc::no_child_process:
                              // no child
                              break;

                           case std::errc::interrupted:
                              signal::dispatch();

                              // We do another turn in the loop
                              return true;

                           default:
                              code::raise::error( code::convert::to::casual( code), "failed to check state of pid: ", exit.pid, " - ", code); 
                        }
                     }
                     else if( result != 0)
                     {
                        exit.pid = strong::process::id{ result};


                        if( WIFEXITED( exit.status))
                        {
                           exit.reason = lifetime::Exit::Reason::exited;
                           exit.status = WEXITSTATUS( exit.status);
                        }
                        else if( WIFSIGNALED( exit.status))
                        {
                           if( WCOREDUMP( exit.status))
                           {
                              exit.reason = lifetime::Exit::Reason::core;
                              exit.status = 0;
                           }
                           else
                           {
                              exit.reason = lifetime::Exit::Reason::signaled;
                              exit.status = WTERMSIG( exit.status);
                           }
                        }
                        else if( WIFSTOPPED( exit.status))
                        {
                           exit.reason = lifetime::Exit::Reason::stopped;
                           exit.status = WSTOPSIG( exit.status);
                        }
                        else if( WIFCONTINUED( exit.status))
                        {
                           exit.reason = lifetime::Exit::Reason::continued;
                           exit.status = 0;
                        }
                     }
                     return false;
                  };

                  while( loop());

                  //log::debug << "wait exit: " << exit << '\n';

                  return exit;
               }

               void wait( const std::vector< strong::process::id>& pids, std::vector< lifetime::Exit>& result)
               {
                  while( result.size() < pids.size())
                  {
                     auto exit = local::wait( strong::process::id{ -1}, 0);

                     if( algorithm::find( pids, exit.pid))
                        result.push_back( std::move( exit));
                     else if( ! exit.pid)
                        return; // No children exists
                  }
               }

               auto terminate = []( const Handle& process)
               {
                  if( process)
                  {
                     message::shutdown::Request request{ handle()};
                     communication::device::blocking::send( process.ipc, request);
                  }
                  else
                     process::terminate( process.pid);

                  return process.pid;
               };

            } // <unnamed>

         } // local

         int wait( strong::process::id pid)
         {
            return local::wait( pid, 0).status;
         }

         std::vector< strong::process::id> terminate( const std::vector< strong::process::id>& pids)
         {
            log::line( verbose::log, "process::terminate pids: ", pids);

            return algorithm::transform_if( pids, 
               []( auto pid){ return pid;}, 
               []( auto pid){ return terminate( pid);});
         }


         bool terminate( strong::process::id pid)
         {
            return signal::send( pid, code::signal::terminate);
         }

         void terminate( const Handle& process)
         {
            if( auto pid = local::terminate( process))
               wait( pid);            
         }

         std::vector< strong::process::id> terminate( const std::vector< Handle>& processes)
         {
            log::line( verbose::log, "process::terminate processes: ", processes);

            auto result = algorithm::transform( processes, local::terminate);
            return algorithm::trim( result, algorithm::remove_if( result, []( auto pid){ return pid.empty();}));
         }



         namespace lifetime
         {
            Exit::operator bool () const { return ! pid.empty();}

            bool Exit::deceased() const
            {
               return reason == Reason::core || reason == Reason::exited || reason == Reason::signaled;
            }

            bool operator == ( strong::process::id pid, const Exit& rhs) { return pid == rhs.pid;}
            bool operator == ( const Exit& lhs, strong::process::id pid) { return pid == lhs.pid;}
            bool operator < ( const Exit& lhs, const Exit& rhs) { return lhs.pid < rhs.pid;}

            std::ostream& operator << ( std::ostream& out, const Exit::Reason& value)
            {
               switch( value)
               {
                  case Exit::Reason::exited: return out << "exited";
                  case Exit::Reason::stopped: return out << "stopped";
                  case Exit::Reason::continued: return out << "continued";
                  case Exit::Reason::signaled: return out <<  "signaled";
                  case Exit::Reason::core: return out << "core";
                  case Exit::Reason::unknown : return out << "unknown";
               }
               return out << "<unknown>";
            }

            std::vector< lifetime::Exit> ended()
            {
               Trace trace{ "process::lifetime::ended"};

               std::vector< lifetime::Exit> terminations;

               while( true)
               {
                  auto exit = local::wait( strong::process::id{ -1});

                  if( exit)
                     terminations.push_back( exit);
                  else
                  {
                     log::line( verbose::log, "terminations: ", terminations);
                     return terminations;
                  }
               }
            }



            std::vector< Exit> wait( const std::vector< strong::process::id>& pids)
            {
               log::line( verbose::log, "process::lifetime::wait pids: ", pids);

               std::vector< Exit> result;

               local::wait( pids, result);

               return result;
            }

            std::vector< Exit> wait( const std::vector< strong::process::id>& pids, platform::time::unit timeout)
            {
               Trace trace{ "common::process::lifetime::wait"};

               log::line( verbose::log, "process::lifetime::wait pids: ", pids, " - timeout: ", timeout);

               if( pids.empty())
                  return {};

               std::vector< Exit> result;
               
               try
               {
                  signal::timer::Scoped alarm( timeout);
                  local::wait( pids, result);
               }
               catch( ...)
               {
                  if( exception::code() != code::signal::alarm)
                     throw;
               }
               return result;
            }

            std::vector< Exit> terminate( const std::vector< Handle>& processes)
            {
               return wait( process::terminate( processes));
            }

            std::vector< Exit> terminate( const std::vector< strong::process::id>& pids)
            {
               return wait( process::terminate( pids));
            }

            std::vector< Exit> terminate( const std::vector< strong::process::id>& pids, platform::time::unit timeout)
            {
               return wait( process::terminate( pids), timeout);
            }

         } // lifetime

      } // process

      Process::Process( const std::string& path, std::vector< std::string> arguments)
         : process::Handle{ process::spawn( path, std::move( arguments))}
      {

      }

      Process::~Process() 
      {
         if( pid)
            exception::guard( [&]()
            {
               Trace trace{ "common::Process::~Process"};
               log::line( verbose::log, "this: ", *this);

                process::terminate( *this);
            });
      }

      void Process::handle( const process::Handle& handle)
      {
         if( pid != handle.pid)
            code::raise::error( code::casual::invalid_argument, "trying to update process with different pids: ", pid, " != ", handle.pid);

         Handle::operator = ( handle);
      }

      void Process::clear()
      {
         Handle::operator = ( {});
      }

   } // common
} // casual


