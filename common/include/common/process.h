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
            inline friend bool operator < ( const Handle& lhs, const Handle& rhs)
            {
               if( lhs.pid == rhs.pid)
                  return lhs.queue < rhs.queue;
               return lhs.pid < rhs.pid;
            }

            friend std::ostream& operator << ( std::ostream& out, const Handle& value);

            struct equal
            {
               struct pid
               {
                  pid( platform::pid_type pid) : m_pid( pid) {}
                  bool operator() ( const Handle& lhs) { return lhs.pid == m_pid;}
               private:
                  platform::pid_type m_pid;
               };
            };

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

         namespace instance
         {
            namespace identity
            {
               const Uuid& broker();

               namespace traffic
               {
                  const Uuid& manager();
               } // traffic
            } // identity

            namespace fetch
            {
               enum class Directive : char
               {
                  wait,
                  direct
               };

               Handle handle( const Uuid& identity, Directive directive);

            } // fetch

            namespace transaction
            {
               namespace manager
               {
                  const Uuid& identity();

                  const Handle& handle();

                  //!
                  //! 'refetch' transaction managers process::Handle.
                  //! @note only for unittest purpose?
                  //!
                  const Handle& refetch();

               } // manager

            } // transaction



         } // instance


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
         platform::pid_type spawn( const std::string& path, std::vector< std::string> arguments);


         platform::pid_type spawn(
            const std::string& path,
            std::vector< std::string> arguments,
            std::vector< std::string> environment);

         //!
         //! Spawn a new application that path describes, and wait until it exit. That is
         //!  - spawn
         //!  - wait
         //!
         //! @param path path to application to be spawned
         //! @param arguments 0..N arguments that is passed to the application
         //! @return exit code from the process
         //!
         int execute( const std::string& path, std::vector< std::string> arguments);


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
         //! @deprecated only used by broker, and should be moved there...
         //!
         file::scoped::Path singleton( std::string queue_id_file);

         //!
         //! Asks broker for the handle to a registered 'singleton' server(process)
         //!
         //! if wait is true (default) broker will not send a response until the requested server
         //!  has register.
         //!
         //! @return a handle to the requested server if found, or a 'null' handle if @p wait is false
         //!  and no server was found
         //!
         Handle singleton( const Uuid& identification, bool wait = true);


         //!
         //! ping a server that owns the @p queue
         //!
         //! @note will block
         //!
         //! @return the process handle
         //!
         Handle ping( platform::queue_id_type queue);

         namespace lifetime
         {
            struct Exit
            {
               enum class Reason : char
               {
                  core,
                  exited,
                  stopped,
                  continued,
                  signaled,
                  unknown,
               };

               platform::pid_type pid = 0;
               int status = 0;
               Reason reason = Reason::unknown;

               explicit operator bool () const;

               //!
               //! @return true if the process life has ended
               //!
               bool deceased() const;


               friend bool operator == ( platform::pid_type pid, const Exit& rhs);
               friend bool operator == ( const Exit& lhs, platform::pid_type pid);
               friend bool operator < ( const Exit& lhs, const Exit& rhs);

               friend std::ostream& operator << ( std::ostream& out, const Exit& terminated);

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & pid;
                  archive & status;
                  archive & reason;
               })

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
            //! @return the terminated l
            //!
            //
            std::vector< Exit> terminate( std::vector< platform::pid_type> pids);
            std::vector< Exit> terminate( std::vector< platform::pid_type> pids, std::chrono::microseconds timeout);

         } // lifetime

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
               for( auto& death : lifetime::terminate( std::move( pids)))
               {
                  state.process( death);
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
