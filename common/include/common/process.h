//!
//! process.h
//!
//! Created on: May 12, 2013
//!     Author: Lazan
//!

#ifndef PROCESS_H_
#define PROCESS_H_

#include "common/platform.h"
#include "common/exception.h"
#include "common/file.h"

#include "common/internal/log.h"
#include "common/algorithm.h"

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
         //! @return the path of the current process
         //!
         const std::string& path();

         //!
         //! Sets the path of the current process
         //!
         void path( const std::string& path);

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
         //! @return pids that did NOT received the signal
         //!
         std::vector< platform::pid_type> terminate( const std::vector< platform::pid_type>& pids);

         //!
         //! Tries to terminate pid
         //!
         bool terminate( platform::pid_type pid);

         //!
         //!
         //!
         file::scoped::Path singleton( std::string queue_id_file);


         namespace lifetime
         {
            struct Exit
            {
               enum class Reason
               {
                  unknown,
                  exited,
                  stopped,
                  signaled,
                  core,
               };

               platform::pid_type pid = 0;
               int status = 0;
               Reason reason = Reason::unknown;

               friend std::ostream& operator << ( std::ostream& out, const Exit& terminated);

            };

            std::vector< Exit> ended();

         }

         namespace children
         {

            //!
            //! Terminate all children that @p state dictates.
            //! When a child is terminated state is updated
            //!
            //! @param state the state object
            //! @param timeout default to 5s
            //!
            template< typename S, typename T = std::chrono::seconds>
            void terminate( S& state, T timeout = std::chrono::seconds( 5))
            {
               //
               // Terminate all processes
               //

               //
               // We remove the pids that is absent
               //
               for( auto pid : process::terminate( state.processes()))
               {
                  state.removeProcess( pid);
               }

               auto processes = state.processes();
               log::internal::debug << "terminating processes: " << range::make( processes) << std::endl;



               auto start = platform::clock_type::now();

               while( state.processes().size() > 0)
               {
                  process::sleep( timeout / 20);

                  for( auto dead : process::lifetime::ended())
                  {
                     log::internal::debug << "pid : " << dead.pid << " terminated" << std::endl;
                     state.removeProcess( dead.pid);
                  }

                  //
                  // It seems that sometimes the signal doesn't "take", so
                  // we fire again...
                  //
                  for( auto pid : process::terminate( state.processes()))
                  {
                     state.removeProcess( pid);
                  }

                  if( platform::clock_type::now() - start > timeout)
                  {
                     throw exception::signal::Timeout{};
                  }
               }
            }

         } // children

      } // process
   } // common
} // casual


#endif /* PROCESS_H_ */
