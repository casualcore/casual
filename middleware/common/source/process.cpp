//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/process.h"
#include "common/exception/system.h"
#include "common/exception/casual.h"

#include "common/file.h"
#include "common/log.h"
#include "common/signal.h"
#include "common/string.h"
#include "common/environment.h"
#include "common/uuid.h"

#include "common/message/domain.h"
#include "common/message/server.h"
#include "common/communication/ipc.h"
#include "common/flag.h"
#include "common/chronology.h"

//
// std
//
#include <algorithm>
#include <functional>
#include <fstream>
#include <system_error>


// TODO: temp
#include <iostream>

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
//#include <fcntl.h>

#include <spawn.h>


#ifdef __APPLE__
   #include <crt_externs.h>
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
                  {
                     throw exception::system::invalid::Argument{ "failed to get the path to the current executable"};
                  }
                  if( path.data()) { return path.data();}
                  return {};
#else
                  return file::name::link( "/proc/self/exe");
#endif
               }

               std::string& path()
               {
                  static std::string path = get_process_path();
                  return path;
               }

               std::string& basename()
               {
                  static std::string basename = file::name::base( local::path());
                  return basename;
               }

               char* const * environment()
               {
                  #ifdef __APPLE__
                      return *::_NSGetEnviron();
                  #else
                     return ::environ;
                  #endif
               }


               namespace instantiated
               {
                  auto pid = strong::process::id{ ::getpid()};
                  
               } // instantiated

               const Handle& handle()
               {
                  static const Handle result{ 
                     instantiated::pid, 
                     communication::ipc::inbound::ipc()};
                  return result;
               }


               namespace instantiated
               {
                  std::string& path = local::path();
                  std::string& basename = local::basename();
                  //const Handle& handle = local::handle();
               } // instantiated



            } // <unnamed>
         } // local

         const std::string& path()
         {
            return local::instantiated::path;
         }

         const std::string& basename()
         {
            return local::instantiated::basename;
         }


         strong::process::id id()
         {
            return local::instantiated::pid;
         }

         const Handle& handle()
         {
            return local::handle();
         }


         bool operator == ( const Handle& lhs, const Handle& rhs)
         {
            return lhs.pid == rhs.pid && lhs.ipc == rhs.ipc;
         }

         bool operator < ( const Handle& lhs, const Handle& rhs)
         {
            return std::tie( lhs.pid, lhs.ipc) < std::tie( rhs.pid, rhs.ipc);
         }

         std::ostream& operator << ( std::ostream& out, const Handle& value)
         {
            return out << "{ pid: " << value.pid << ", ipc: " << value.ipc << '}';
         }



         const Uuid& uuid()
         {
            static const Uuid singleton = uuid::make();
            return singleton;
         }

         void sleep( common::platform::time::unit time)
         {
            log::line( verbose::log, "process::sleep time: ", time);

            timespec posix_time;
            posix_time.tv_sec = std::chrono::duration_cast< std::chrono::seconds>( time).count();
            posix_time.tv_nsec = std::chrono::duration_cast< std::chrono::nanoseconds>(
                  time - std::chrono::seconds{ posix_time.tv_sec}).count();

            //
            // We check signals before we sleep
            //
            signal::handle();

            if( nanosleep( &posix_time, nullptr) == -1)
            {
               switch( code::last::system::error())
               {
                  case code::system::interrupted:
                  {
                     signal::handle();
                     break;
                  }
                  default:
                  {
                     exception::system::throw_from_errno();
                  }
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


            Sleep::Pattern::Pattern( common::platform::time::unit time, platform::size::type quantity)
               : m_time{ time}, m_quantity{ local::check_infinity( quantity)}
            {}

            Sleep::Pattern::Pattern( common::platform::time::unit time, infinite_quantity)
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

            std::ostream& operator << ( std::ostream& out, const Sleep::Pattern& value)
            {
               return out << "{ duration: " << value.m_time.count() << "us, quantity: " << value.m_quantity
                     << '}';
            }


            Sleep::Sleep( std::initializer_list< Pattern> pattern) : m_pattern( std::move( pattern))
            {
               //
               // We reverse the patterns so we can go from the back
               //
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

            std::ostream& operator << ( std::ostream& out, const Sleep& value)
            {
               return out << "{ pattern: " << range::make( value.m_pattern) << '}';
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
                     //
                     // Appends from current process environment, if it's not
                     // already overridden.
                     //
                     void environment( std::vector< std::string>& variables)
                     {
                        //
                        // Since we're reading environment variables we need to lock
                        //
                        std::unique_lock< std::mutex> lock{ environment::variable::mutex()};

                        auto current = local::environment();

                        while( (*current) != nullptr)
                        {
                           std::string variable{ *current};

                           auto found =  algorithm::find_if( variables, [&variable]( const std::string& v){
                              return algorithm::equal(
                                 std::get< 0>( algorithm::divide( variable, '=')),
                                 std::get< 0>( algorithm::divide( v, '='))
                              );
                           });

                           if( ! found)
                           {
                              variables.push_back( std::move( variable));
                           }

                           ++current;
                        }
                     }
                  } // copy


               } // current

               namespace C
               {
                  constexpr auto string_data = []( auto& v){ return v.data();};

                  std::vector< const char*> environment( std::vector< std::string>& environment)
                  {
                     std::vector< const char*> result;
                     result.resize( environment.size() + 1);

                     std::transform(
                        std::begin( environment),
                        std::end( environment),
                        std::begin( result),
                        C::string_data);

                     result.push_back( nullptr);

                     return result;
                  }

                  std::vector< const char*> arguments( const std::string& path, std::vector< std::string>& arguments)
                  {
                     std::vector< const char*> c_arguments;

                     //
                     // think we must add application-name as first argument...
                     //
                     c_arguments.push_back( path.c_str());


                     algorithm::transform( arguments, c_arguments, C::string_data);

                     //
                     // Null end
                     //
                     c_arguments.push_back( nullptr);

                     return c_arguments;
                  }

               } // C

               namespace spawn
               {
                  struct Attributes : traits::uncopyable
                  {
                     Attributes()
                     {
                        check_error( posix_spawnattr_init( &m_attributes), "posix_spawnattr_init");

                        //
                        // We try to eliminate signals to propagate to children by it self...
                        // we don't need to set groupid with posix_spawnattr_setpgroup since the default is 0.
                        //
                        check_error( posix_spawnattr_setflags( &m_attributes, POSIX_SPAWN_SETPGROUP), "posix_spawnattr_setflags");
                     }

                     ~Attributes()
                     {
                        posix_spawnattr_destroy( &m_attributes);
                     }

                     posix_spawnattr_t* get() { return &m_attributes;}

                  private:

                     void check_error( int code, const char* message)
                     {
                        if( code != 0)
                           exception::system::throw_from_code( code);
                     };

                     posix_spawnattr_t m_attributes;
                  };
               } // spawn

            } // <unnamed>
         } // local


      strong::process::id spawn(
            std::string path,
            std::vector< std::string> arguments,
            std::vector< std::string> environment)
         {
            Trace trace{ "process::spawn"};

            path = environment::string( std::move( path));

            //
            // check if path exist and process has permission to execute it.
            // could still go wrong, since we don't know if the path will actually execute,
            // but we'll probably get rid of most of the errors (due to bad configuration and such)
            //
            if( ! file::permission::execution( path))
            {
               throw exception::system::invalid::Argument( string::compose( "spawn failed - path: ", path));
            }

            //
            // We need to expand environment and arguments
            //
            {
               for( auto& argument : arguments)
               {
                  argument = environment::string( argument);
               }

               for( auto& variable : environment)
               {
                  variable = environment::string( variable);
               }
            }

            
            log::line( log::debug, "process::spawn ", path, ' ', arguments);
            log::line( verbose::log, "environment: ", environment);

            //
            // Append current, if not "overridden"
            //
            local::current::copy::environment( environment);


            strong::process::id pid;

            {
               local::spawn::Attributes attributes;

               //
               // prepare c-style arguments and environment
               //
               auto c_arguments = local::C::arguments( path, arguments);
               auto c_environment = local::C::environment( environment);

               platform::process::native::type native_pid;

               auto status =  posix_spawnp(
                     &native_pid,
                     path.c_str(),
                     nullptr,
                     attributes.get(),
                     const_cast< char* const*>( c_arguments.data()),
                     const_cast< char* const*>( c_environment.data())
                     );
               switch( status)
               {
                  case 0:
                     break;
                  default:
                     exception::system::throw_from_code( status);
               }

               pid = strong::process::id{ native_pid};
            }

            //
            // We try to minimize the glitch where the spawned process does not
            // get signals for a short period of time. We need to block so we don't
            // get child-terminate signals (or other signals for that matter...)
            //
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


         strong::process::id spawn( const std::string& path, std::vector< std::string> arguments)
         {
            return spawn( path, std::move( arguments), {});
         }



         int execute( const std::string& path, std::vector< std::string> arguments)
         {
            return wait( spawn( path, std::move( arguments)));
         }


         namespace local
         {
            namespace
            {
               lifetime::Exit wait( strong::process::id pid, int flags = WNOHANG)
               {


                  auto handle_signal = [](){
                     try
                     {
                        signal::handle();
                     }
                     catch( const exception::signal::child::Terminate&)
                     {
                        // no-op
                     }
                  };

                  lifetime::Exit exit;

                  auto loop = [&]()
                  {
                     handle_signal();

                     auto result = waitpid( pid.value(), &exit.status, flags);

                     if( result == -1)
                     {
                        switch( code::last::system::error())
                        {
                           case code::system::no_child_process:
                           {
                              // no child
                              break;
                           }
                           case code::system::interrupted:
                           {
                              handle_signal();

                              //
                              // We do another turn in the loop
                              //
                              return true;
                           }
                           default:
                           {
                              log::line( log::category::error, "failed to check state of pid: ", exit.pid, " - ", code::last::system::error());
                              exception::system::throw_from_errno();
                           }
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
                     {
                        result.push_back( std::move( exit));
                     }
                     else if( ! exit.pid)
                     {
                        //
                        // No children exists
                        //
                        return;
                     }
                  }
               }

            } // <unnamed>

         } // local

         int wait( strong::process::id pid)
         {
            //
            // We'll only handle child signals.
            //
            //signal::thread::scope::Mask block{ signal::set::filled( { signal::Type::child})};

            return local::wait( pid, 0).status;
         }




         std::vector< strong::process::id> terminate( const std::vector< strong::process::id>& pids)
         {
            log::line( verbose::log, "process::terminate pids: ", pids);

            std::vector< strong::process::id> result;
            for( auto pid : pids)
            {
               if( terminate( pid))
               {
                  result.push_back( pid);
               }
            }
            return result;
         }



         bool terminate( strong::process::id pid)
         {
            return signal::send( pid, signal::Type::terminate);
         }

         void terminate( const Handle& process)
         {
            if( process)
            {
               message::shutdown::Request request;
               request.process = handle();
               communication::ipc::blocking::send( process.ipc, request);
            }
            else if( process.pid)
            {
               terminate( process.pid);
            }
            else
            {
               return;
            }
            wait( process.pid);
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
                  case Exit::Reason::exited: out << "exited"; break;
                  case Exit::Reason::stopped: out << "stopped"; break;
                  case Exit::Reason::continued: out << "continued"; break;
                  case Exit::Reason::signaled: out <<  "signaled"; break;
                  case Exit::Reason::core: out <<  "core"; break;
                  default: out << "unknown"; break;
               }
               return out;
            }

            std::ostream& operator << ( std::ostream& out, const Exit& terminated)
            {
               return out << "{ pid: " << terminated.pid << ", reason: " << terminated.reason << '}';
            }

            std::vector< lifetime::Exit> ended()
            {
               Trace trace{ "process::lifetime::ended"};

               //
               // We'll only handle child signals.
               //
               signal::thread::scope::Mask block{ signal::set::filled( signal::Type::child)};

               std::vector< lifetime::Exit> terminations;

               while( true)
               {
                  auto exit = local::wait( strong::process::id{ -1});
                  if( exit)
                  {
                     terminations.push_back( exit);
                  }
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

            std::vector< Exit> wait( const std::vector< strong::process::id>& pids, common::platform::time::unit timeout)
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
               catch( const exception::signal::Timeout&)
               {

               }
               return result;
            }


            std::vector< Exit> terminate( const std::vector< strong::process::id>& pids)
            {
               return wait( process::terminate( pids));
            }

            std::vector< Exit> terminate( const std::vector< strong::process::id>& pids, common::platform::time::unit timeout)
            {
               return wait( process::terminate( pids), timeout);
            }

         } // lifetime


      } // process
   } // common
} // casual


