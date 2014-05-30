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


#include <crt_externs.h>


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
            usleep( time.count());
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



            platform::pid_type pid;

            if( posix_spawnp(
                  &pid,
                  path.c_str(),
                  nullptr,
                  nullptr, //&attributes,
                  const_cast< char* const*>( c_arguments.data()),
                  *_NSGetEnviron()// environ //const_cast< char* const*>( c_environment.data())
                  ) != 0)
            {
               throw exception::NotReallySureWhatToNameThisException();
            }

            log::internal::debug << "spawned pid: " << pid << " - " << path << " " << string::join( arguments, " ") << std::endl;

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
                     exit.why = lifetime::Exit::Why::exited;
                     exit.status = WEXITSTATUS( exit.status);
                  }
                  else if( WIFSIGNALED( exit.status))
                  {
                     if( WCOREDUMP( exit.status))
                     {
                        exit.why = lifetime::Exit::Why::core;
                        exit.status = 0;
                     }
                     else
                     {
                        exit.why = lifetime::Exit::Why::signaled;
                        exit.status = WTERMSIG( exit.status);
                     }
                  }
                  else if( WIFSTOPPED( exit.status))
                  {
                     exit.why = lifetime::Exit::Why::stopped;
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




         void terminate( const std::vector< platform::pid_type>& pids)
         {
            for( auto pid : pids)
            {
               terminate( pid);
            }
         }



         void terminate( platform::pid_type pid)
         {
            signal::send( pid, platform::cSignal_Terminate);
         }

         std::vector< lifetime::Exit> lifetime::ended()
         {
            std::vector< lifetime::Exit> result;

            Exit exit;

            while( local::wait( exit, -1))
            {
               result.push_back( exit);
            }

            return result;
            //return state();
         }



      } // process
   } // common
} // casual


