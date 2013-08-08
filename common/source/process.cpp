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
#include "common/logger.h"
#include "common/trace.h"
#include "common/signal.h"
#include "common/string.h"


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



namespace casual
{
   namespace common
   {
      namespace process
      {



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
                  std::bind( &std::string::data, std::placeholders::_1));

            c_arguments.push_back( nullptr);

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


            platform::pid_type pid = fork();

            switch( pid)
            {
               case -1:
               {
                  throw exception::NotReallySureWhatToNameThisException();
               }
               case 0:
               {
                  // In the child process.  Iterate through all possible file descriptors
                  // and explicitly close them.
                  /*
                  long maxfd = sysconf( _SC_OPEN_MAX);
                  for( long filedescripor = 0; filedescripor < maxfd; ++filedescripor)
                  {
                     close( filedescripor);
                  }
                  */

                  //
                  // Close other end of the pipe, and make sure it's own end is closed
                  // when exited
                  //
                  //close( pipeDesriptor[ 0]);
                  //fcntl( pipeDesriptor[ 1], F_SETFD, FD_CLOEXEC);

                  //
                  // executed by shild, lets start process
                  //
                  execvp( path.c_str(), const_cast< char* const*>( c_arguments.data()));

                  //std::cerr << "errno: " << error::stringFromErrno() << std::endl;

                  //
                  // If we reach this, the execution failed...
                  // We pipe to the parent
                  //
                  //local::writeToPipe( pipeDesriptor[ 1], "execution filed");
                  _exit( 222);


                  // We can't throw, we log it...
                  //
                  //logger::error << "failed to execute " + path + " - " + error::stringFromErrno();
                  // TODO: hack, use pipe to get real exit from the child later on...

                  break;
               }
               default:
               {
                  //
                  // Parent process
                  //

                  //
                  // Close other end
                  //
                  //close( pipeDesriptor[ 1]);

                  //
                  // read from child
                  //
                  //auto result = local::readFromPipe( pipeDesriptor[ 0]);
                  //close( pipeDesriptor[ 0]);

                  //if( result.empty())
                  {

                     //
                     // We have started the process, hopefully...
                     //
                     logger::information << "spawned: " << path << " " << string::join( arguments, " ");
                  }
                  /*
                  else
                  {
                     logger::error << "failed to spawn: " << path << " " << string::join( arguments, " ") << " - " << result;
                  }
                  */
                  break;
               }
            }

            return pid;
         }





         int wait( platform::pid_type pid)
         {
            int status = 0;
            waitpid( pid, &status, 0);

            if( ! WIFEXITED( status))
            {
               throw exception::NotReallySureWhatToNameThisException( "the child did not terminated normally");
            }
            // TODO: use pipes to do this stuff...
            else if( WEXITSTATUS( status) == 222)
            {
               throw exception::NotReallySureWhatToNameThisException( "the child could not be executed");
            }

            return WEXITSTATUS( status);
         }

         int execute( const std::string& path, const std::vector< std::string>& arguments)
         {
            return wait( spawn( path, arguments));
         }


         namespace local
         {
            platform::pid_type wait()
            {
               const platform::pid_type anyChild = -1;
               int status = 0;
               platform::pid_type pid = waitpid( anyChild, &status, WNOHANG);

               if( pid == -1)
               {
                  if( errno == error::cNoChildProcesses)
                  {
                     pid = 0;
                  }
                  else
                  {
                     logger::error << "failed to check state of child process errno: " << errno << " - "<< error::stringFromErrno();
                     throw exception::NotReallySureWhatToNameThisException();
                  }
               }
               return pid;
            }
         }

         std::vector< platform::pid_type> terminated()
         {
            std::vector< platform::pid_type> result;

            platform::pid_type pid = 0;

            while( ( pid = local::wait()) != 0)
            {
               result.push_back( pid);
            }

            return result;
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

      } // process
   } // common
} // casual

