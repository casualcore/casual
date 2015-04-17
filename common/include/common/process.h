//!
//! process.h
//!
//! Created on: May 12, 2013
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_PROCESS_H_
#define CASUAL_COMMON_PROCESS_H_

#include "common/platform.h"
#include "common/exception.h"
#include "common/file.h"

#include "common/internal/log.h"
#include "common/algorithm.h"

#include "common/marshal/marshal.h"

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
      struct Uuid;

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
         //! Holds pid and ipc-queue for a given process
         //!
         struct Handle
         {
            Handle() = default;
            Handle( platform::pid_type pid, platform::queue_id_type queue) : pid( pid), queue( queue) {}

            platform::pid_type pid = 0;
            platform::queue_id_type queue = 0;


            friend bool operator == ( const Handle& lhs, const Handle& rhs);
            inline friend bool operator != ( const Handle& lhs, const Handle& rhs) { return !( lhs == rhs);}
            friend std::ostream& operator << ( std::ostream& out, const Handle& value);

            explicit operator bool() const
            {
               return pid != 0 && queue != 0;
            }

            CASUAL_CONST_CORRECT_MARSHAL(
            {
               archive & pid;
               archive & queue;
            })

         };

         //!
         //! @return the process handle for current process
         //!
         Handle handle();



         //!
         //! @return the uuid for this process.
         //! used as a unique id over time
         //!
         const Uuid& uuid();

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


         platform::pid_type spawn(
            const std::string& path,
            const std::vector< std::string>& arguments,
            const std::vector< std::string>& environment);

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
         //! @return pids that did received the signal
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

               explicit operator bool () { return pid != 0;}


               friend bool operator == ( platform::pid_type pid, const Exit& rhs) { return pid == rhs.pid;}
               friend bool operator == ( const Exit& lhs, platform::pid_type pid) { return pid == lhs.pid;}

               friend std::ostream& operator << ( std::ostream& out, const Exit& terminated);


            };

            std::vector< Exit> ended();


            //!
            //!
            //!
            std::vector< Exit> wait( const std::vector< platform::pid_type> pids);
            std::vector< Exit> wait( const std::vector< platform::pid_type> pids, std::chrono::microseconds timeout);


            //!
            //! Terminates and waits for the termination.
            //!
            //! @return the terminated pids
            //!
            //
            std::vector< platform::pid_type> terminate( std::vector< platform::pid_type> pids);
            std::vector< platform::pid_type> terminate( std::vector< platform::pid_type> pids, std::chrono::microseconds timeout);

         }

         namespace children
         {


            //!
            //! Terminate all children that @p pids dictates.
            //! When a child is terminated state is updated
            //!
            //! @param state the state object
            //! @param pids to terminate
            //!
            template< typename S>
            void terminate( S& state, std::vector< platform::pid_type> pids)
            {
               for( auto& pid : lifetime::terminate( std::move( pids)))
               {
                  state.removeProcess( pid);
               }
            }

            //!
            //! Terminate all children that @p state dictates.
            //! When a child is terminated state is updated
            //!
            //! @param state the state object
            //!
            template< typename S>
            void terminate( S& state)
            {
               //
               // Terminate all processes
               //

               terminate( state, state.processes());
            }


         } // children

      } // process
   } // common
} // casual


#endif /* PROCESS_H_ */
