//!
//! process.h
//!
//! Created on: May 12, 2013
//!     Author: Lazan
//!

#ifndef PROCESS_H_
#define PROCESS_H_

#include "common/platform.h"

//
// std
//
#include <string>
#include <vector>
#include <chrono>

namespace casual
{
   namespace common
   {
      namespace process
      {

         //!
         //!
         //!
         const std::string& path();

         //!
         //! @return process id (pid) for current process.
         //!
         platform::pid_type id();

         //!
         //! Sleep for a while
         //!
         //! @param time numbers of microseconds to sleep
         //!
         void sleep( std::chrono::microseconds time);

         //!
         //! Sleep for an arbitrary duration
         //!
         //! Example:
         //! ~~~~~~~~~~~~~~~{.cpp}
         //! // sleep for 20 seconds
         //! process::sleep( std::chrono::seconds( 20));
         //!
         //! // sleep for 2 minutes
         //! process::sleep( std::chrono::minutes( 2));
         //! ~~~~~~~~~~~~~~~
         //!
         template< typename R, typename P>
         void sleep( std::chrono::duration< R, P> time)
         {
            sleep( std::chrono::duration_cast< std::chrono::microseconds>( time));
         }


         //!
         //! Spawn a new application that path describes
         //!
         //! @param path path to application to be spawned
         //! @param arguments 0..N arguments that is passed to the application
         //! @return process id of the spawned process
         //!
         platform::pid_type spawn( const std::string& path, const std::vector< std::string>& arguments);

         //!
         //! Spawn a new application that path describes, and wait until it exit. That is
         //!  - spawn
         //!  - wait
         //!
         //! @param path path to application to be spawned
         //! @param arguments 0..N arguments that is passed to the application
         //! @return exit code from the process
         //!
         int execute( const std::string& path, const std::vector< std::string>& arguments);


         //!
         //! check if there are child processes that has been terminated
         //!
         //! @return 0..N terminated process id's
         //!
         //std::vector< platform::pid_type> terminated();

         //!
         //! Wait for a specific process to terminate.
         //!
         //! @return return code from process
         //!
         int wait( platform::pid_type pid);

         //!
         //! Tries to terminate pids
         //!
         void terminate( const std::vector< platform::pid_type>& pids);

         //!
         //! Tries to terminate pid
         //!
         void terminate( platform::pid_type pid);


         struct lifetime
         {
            struct Exit
            {
               enum class Why
               {
                  unknown,
                  exited,
                  stopped,
                  signaled,
                  core,
               };

               platform::pid_type pid = 0;
               int status = 0;
               Why why = Why::unknown;

               std::string string() const
               {
                  std::string result;
                  result = "pid: " + std::to_string( pid) + " why: ";
                  switch( why)
                  {
                     case Why::unknown: result += "unknown"; break;
                     case Why::exited: result += "exited"; break;
                     case Why::stopped: result += "stopped"; break;
                     case Why::signaled: result += "signaled"; break;
                     case Why::core: result += "core"; break;
                  }
                  return result;
               }

            };

            static std::vector< Exit> ended();
            //static void clear();

            // called by signal...
            static void death();

         private:
            //static std::vector< Exit>& state();
         };


      } // process
   } // common
} // casual


#endif /* PROCESS_H_ */
