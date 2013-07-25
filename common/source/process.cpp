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


//
// std
//
#include <algorithm>
#include <functional>

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

namespace casual
{
   namespace common
   {
      namespace process
      {

         void sleep( std::chrono::microseconds time)
         {
            usleep( time.count());
         }


         platform::pid_type spawn( const std::string& path, const std::vector< std::string>& arguments)
         {

            if( ! file::exists( path))
            {
               throw exception::FileNotExist( path);
            }


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


            platform::pid_type pid = fork();

            switch( pid)
            {
               case -1:
               {
                  throw exception::NotReallySureWhatToNameThisException();
               }
               case 0:
               {
                  //
                  // executed by shild, lets start process
                  //
                  execv( path.c_str(), const_cast< char* const*>( c_arguments.data()));

                  //
                  // If we reach this, the execution failed...
                  // We can't throw, we log it...
                  //
                  logger::error << "failed to execute " + path + " - " + error::stringFromErrno();
                  std::exit( errno);
                  break;
               }
               default:
               {
                  //
                  // We have started the process, hopefully...
                  //
                  logger::information << path << " spawned - #arguments: " << arguments.size();
                  break;
               }
            }

            return pid;
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

         platform::pid_type wait( platform::pid_type pid)
         {
            int status = 0;
            return waitpid( pid, &status, 0);
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

