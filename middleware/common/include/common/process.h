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
         platform::pid::type id();


         //!
         //! Holds pid and ipc-queue for a given process
         //!
         struct Handle
         {
            Handle() = default;
            Handle( platform::pid::type pid, platform::ipc::id::type queue) : pid( pid), queue( queue) {}

            platform::pid::type pid = 0;
            platform::ipc::id::type queue = 0;


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
                  pid( platform::pid::type pid) : m_pid( pid) {}
                  bool operator() ( const Handle& lhs) { return lhs.pid == m_pid;}
               private:
                  platform::pid::type m_pid;
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
         //! @throws exception::signal::* when a signal is received
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
         //! @throws exception::signal::* when a signal is received
         //!
         template< typename R, typename P>
         void sleep( std::chrono::duration< R, P> time)
         {
            sleep( std::chrono::duration_cast< std::chrono::microseconds>( time));
         }

         namespace pattern
         {
            struct Sleep
            {
               struct Pattern
               {

                  Pattern( std::chrono::microseconds time, std::size_t quantity);

                  template< typename R, typename P>
                  Pattern( std::chrono::duration< R, P> time, std::size_t quantity)
                   : Pattern{ std::chrono::duration_cast< std::chrono::microseconds>( time), quantity}
                   {}

                   Pattern();

                  std::chrono::microseconds time;
                  std::size_t quantity = 0;
               };

               Sleep( std::vector< Pattern> pattern);
               Sleep( std::initializer_list< Pattern> pattern);

               void operator () ();


            private:

               std::vector< Pattern> m_pattern;
               std::size_t m_offset = 0;
            };

         } // pattern




         //!
         //! Spawn a new application that path describes
         //!
         //! @param path path to application to be spawned
         //! @param arguments 0..N arguments that is passed to the application
         //! @return process id of the spawned process
         //!
         platform::pid::type spawn( const std::string& path, std::vector< std::string> arguments);


         platform::pid::type spawn(
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
         int wait( platform::pid::type pid);


         //!
         //! Tries to terminate pids
         //!
         //! @return pids that did received the signal
         //!
         std::vector< platform::pid::type> terminate( const std::vector< platform::pid::type>& pids);

         //!
         //! Tries to terminate pid
         //!
         bool terminate( platform::pid::type pid);

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
         //! @param pid the pid to lookup
         //!
         //! @return handle to the process, if not found Handle::pid/queue will be 0
         //!
         Handle lookup( platform::pid::type pid, bool wait = true);


         //!
         //! ping a server that owns the @p queue
         //!
         //! @note will block
         //!
         //! @return the process handle
         //!
         Handle ping( platform::ipc::id::type queue);

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

               platform::pid::type pid = 0;
               int status = 0;
               Reason reason = Reason::unknown;

               explicit operator bool () const;

               //!
               //! @return true if the process life has ended
               //!
               bool deceased() const;


               friend bool operator == ( platform::pid::type pid, const Exit& rhs);
               friend bool operator == ( const Exit& lhs, platform::pid::type pid);
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
            std::vector< Exit> wait( const std::vector< platform::pid::type> pids);
            std::vector< Exit> wait( const std::vector< platform::pid::type> pids, std::chrono::microseconds timeout);


            //!
            //! Terminates and waits for the termination.
            //!
            //! @return the terminated l
            //!
            //
            std::vector< Exit> terminate( std::vector< platform::pid::type> pids);
            std::vector< Exit> terminate( std::vector< platform::pid::type> pids, std::chrono::microseconds timeout);

         } // lifetime

         namespace children
         {


            //!
            //! Terminate all children that @p pids dictates.
            //! When a child is terminated callback is called
            //!
            //! @param callback the callback object
            //! @param pids to terminate
            //!
            template< typename C>
            void terminate( C&& callback, std::vector< platform::pid::type> pids)
            {
               for( auto& death : lifetime::terminate( std::move( pids)))
               {
                  callback( death);
               }
            }


         } // children


         //!
         //! Connect the process @handle to the local domain. That is,
         //! let the domain know which ipc-queue is bound to which pid
         //!
         //! @param handle
         //!
         void connect( const Handle& handle);

         //!
         //! Connect the current process to the local domain. That is,
         //! let the domain know which ipc-queue is bound to which pid
         //!
         void connect();

      } // process
   } // common
} // casual


#endif /* PROCESS_H_ */
