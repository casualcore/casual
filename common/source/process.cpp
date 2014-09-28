//!
//! process.cpp
//!
//! Created on: May 12, 2013
//!     Author: Lazan
//!

#include "common/process.h"
#include "common/exception.h"
#include "common/error.h"
#include "common/file.h"
#include "common/internal/log.h"
#include "common/internal/trace.h"
#include "common/signal.h"
#include "common/string.h"
#include "common/environment.h"


//
// std
//
#include <algorithm>
#include <functional>

// TODO: temp
#include <iostream>

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
//#include <fcntl.h>

#include <spawn.h>


#ifdef __APPLE__
   #include <crt_externs.h>
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
               std::string getProcessPath()
               {
                  if( environment::variable::exists( "_"))
                  {
                     return environment::variable::get( "_");
                  }

                  return std::string{};
               }

               std::string& path()
               {
                  static std::string path = getProcessPath();
                  return path;
               }

               char* const * environment()
               {
                  #ifdef __APPLE__
                      return *::_NSGetEnviron();
                  #else
                     return ::environ;
                  #endif
               }


            } // <unnamed>
         } // local

         const std::string& path()
         {
            return local::path();
         }

         void path( const std::string& path)
         {
            local::path() = path;
         }


         platform::pid_type id()
         {
            static const platform::pid_type pid = getpid();
            return pid;
         }

         void sleep( std::chrono::microseconds time)
         {
            std::this_thread::sleep_for( time);
         }


         namespace local
         {
            namespace
            {
               std::string readFromPipe( int filedescripor)
               {
                  std::string result;

                  auto stream = fdopen( filedescripor, "r");

                  if( stream)
                  {
                     char c = 0;
                     while( ( c = fgetc( stream)) != EOF)
                     {
                        result.push_back( c);
                     }

                     fclose( stream);
                  }
                  else
                  {
                     result = "failed to open read stream for " + std::to_string( filedescripor);
                  }

                  return result;
               }

               void writeToPipe( int filedescripor, const std::string& message)
               {
                  auto stream = fdopen( filedescripor, "w");
                  if( stream)
                  {
                     fprintf( stream, message.c_str());
                     fclose( stream);
                  }
                  else
                  {
                     std::cerr << "failed to open write stream for " + std::to_string( filedescripor) << std::endl;
                  }
               }
            } //
         } // local

         platform::pid_type spawn( const std::string& path, const std::vector< std::string>& arguments)
         {

            //
            // prepare arguments
            //
            std::vector< const char*> c_arguments;

            //
            // think we must add application-name as first argument...
            //
            c_arguments.push_back( path.data());

            std::transform(
                  std::begin( arguments),
                  std::end( arguments),
                  std::back_inserter( c_arguments),
                  std::mem_fn( &std::string::data));

            c_arguments.push_back( nullptr);


            std::vector< const char*> c_environment;
            c_environment.push_back( nullptr);



            //std::vector< const char*> c_arguments;

            /*
            int pipeDesriptor[2];
            if( pipe( pipeDesriptor) == -1)
            {
               throw exception::NotReallySureWhatToNameThisException( "failed to create pipe");
            }
            */

            // TODO: temp
            //local::writeToPipe( pipeDesriptor[ 1], "hej");
            //std::cerr << "local::readFromPipe: " << local::readFromPipe( pipeDesriptor[ 0]) << std::endl;


            posix_spawnattr_t attributes;

            posix_spawnattr_init( &attributes);



            platform::pid_type pid;

            auto status =  posix_spawnp(
                  &pid,
                  path.c_str(),
                  nullptr,
                  &attributes,
                  const_cast< char* const*>( c_arguments.data()),
                  local::environment()// environ //const_cast< char* const*>( c_environment.data())
                  );
            switch( status)
            {
               case 0:
                  break;
               default:
                  throw exception::invalid::Argument( "spawn failed for: " + path + " - " + error::string( status));
            }
            return pid;
         }







         int execute( const std::string& path, const std::vector< std::string>& arguments)
         {
            return wait( spawn( path, arguments));
         }


         namespace local
         {
            bool wait( lifetime::Exit& exit, platform::pid_type pid, int flags = WNOHANG)
            {
               exit.pid = waitpid( pid, &exit.status, flags);

               if( exit.pid == -1)
               {
                  if( errno == error::cNoChildProcesses)
                  {

                  }
                  else
                  {
                     log::error << "failed to check state of pid: " << exit.pid << " - " << error::string() << std::endl;
                     throw exception::NotReallySureWhatToNameThisException( error::string());
                  }
               }
               else if( exit.pid != 0)
               {
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
                  return true;
               }
               return false;
            }
         } // local

         int wait( platform::pid_type pid)
         {
            lifetime::Exit exit;

            local::wait( exit, pid, 0);

            return exit.status;
         }




         std::vector< platform::pid_type> terminate( const std::vector< platform::pid_type>& pids)
         {
            std::vector< platform::pid_type> result;
            for( auto pid : pids)
            {
               if( ! terminate( pid))
               {
                  result.push_back( pid);
               }
            }
            return result;
         }



         bool terminate( platform::pid_type pid)
         {
            return signal::send( pid, platform::cSignal_Terminate);
         }

         namespace lifetime
         {

            std::ostream& operator << ( std::ostream& out, const Exit& terminated)
            {
               out << "{pid: " << terminated.pid << " terminated - reason: ";
               switch( terminated.reason)
               {
                  case Exit::Reason::unknown: out << "unknown"; break;
                  case Exit::Reason::exited: out << "exited"; break;
                  case Exit::Reason::stopped: out << "stopped"; break;
                  case Exit::Reason::signaled: out <<  "signaled"; break;
                  case Exit::Reason::core: out <<  "core"; break;
               }
               return out << '}';
            }

            std::vector< lifetime::Exit> ended()
            {
               std::vector< lifetime::Exit> result;

               Exit exit;

               while( local::wait( exit, -1))
               {
                  result.push_back( exit);
               }

               return result;
            }
         } // lifetime



      } // process
   } // common
} // casual


