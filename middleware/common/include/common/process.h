//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/strong/id.h"
#include "common/file.h"
#include "common/uuid.h"

#include "common/algorithm.h"
#include "common/environment/variable.h"

#include "common/serialize/macro.h"



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

         //! @return the path of the current process
         const std::string& path();

         //! @return the basename of the current process
         const std::string& basename();

         //! Holds pid and ipc-queue for a given process
         struct Handle
         {

            Handle() = default;
            inline explicit Handle( strong::process::id pid) : pid{ pid} {}
            inline Handle( strong::process::id pid, strong::ipc::id ipc) : pid( pid),  ipc( std::move( ipc))  {}

            strong::process::id pid;

            //! unique identifier for this process
            strong::ipc::id ipc;

            friend bool operator == ( const Handle& lhs, const Handle& rhs);
            inline friend bool operator != ( const Handle& lhs, const Handle& rhs) { return !( lhs == rhs);}
            friend bool operator < ( const Handle& lhs, const Handle& rhs);

            //! extended equality
            inline friend bool operator == ( const Handle& lhs, const strong::process::id& rhs) { return lhs.pid == rhs;}
            inline friend bool operator == ( const strong::process::id& lhs, const Handle& rhs) { return rhs == lhs;}
            inline friend bool operator == ( const Handle& lhs, const strong::ipc::id& rhs) { return lhs.ipc == rhs;}
            inline friend bool operator == ( const strong::ipc::id& lhs, const Handle& rhs) { return rhs == lhs;}

            inline explicit operator bool() const
            {
               return pid && ipc;
            }

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               CASUAL_SERIALIZE( pid);
               CASUAL_SERIALIZE( ipc);
            })
         };

         //! @return the process handle for current process
         const Handle& handle();

         //! @return process id (pid) for current process.
         strong::process::id id();

         inline strong::process::id id( const Handle& handle) { return handle.pid;}
         inline strong::process::id id( strong::process::id pid) { return pid;}

         //! Sleep for a while
         //!
         //! @throws exception::signal::* when a signal is received
         //!
         //! @param time numbers of microseconds to sleep
         void sleep( platform::time::unit time);

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
         //! @throws exception::system::Interupted if a signal is received
         template< typename R, typename P>
         void sleep( std::chrono::duration< R, P> time)
         {
            sleep( std::chrono::duration_cast< platform::time::unit>( time));
         }

         namespace pattern
         {
            struct Sleep
            {
               struct Pattern
               {
                  struct infinite_quantity {}; 

                  Pattern( platform::time::unit time, platform::size::type quantity);
                  Pattern( platform::time::unit time, infinite_quantity);

                  template< typename R, typename P>
                  Pattern( std::chrono::duration< R, P> time, platform::size::type quantity)
                   : Pattern{ std::chrono::duration_cast< platform::time::unit>( time), quantity}
                  {}

                  template< typename R, typename P>
                  Pattern( std::chrono::duration< R, P> time, infinite_quantity)
                   : Pattern{ std::chrono::duration_cast< platform::time::unit>( time), infinite_quantity{}}
                  {}

                  bool done();

                  CASUAL_LOG_SERIALIZE(
                  {
                     CASUAL_SERIALIZE_NAME( m_time, "time");
                     CASUAL_SERIALIZE_NAME( m_quantity, "quantity");
                  })

               private:
                  platform::time::unit m_time;
                  platform::size::type m_quantity = 0;
               };
               
               Sleep( std::initializer_list< Pattern> pattern);

               bool operator () ();

               // for logging only
               CASUAL_LOG_SERIALIZE(
               {
                  CASUAL_SERIALIZE_NAME( m_pattern, "pattern");
               })

            private:
               std::vector< Pattern> m_pattern;
            };

         } // pattern

         //! Spawn a new application that path describes
         //!
         //! @param path path to application to be spawned
         //! @param arguments 0..N arguments that is passed to the application
         //! @return process id of the spawned process
         strong::process::id spawn( std::string path, std::vector< std::string> arguments);


         strong::process::id spawn(
            std::string path,
            std::vector< std::string> arguments,
            std::vector< environment::Variable> environment);

         //! Spawn a new application that path describes, and wait until it exit. That is
         //!  - spawn
         //!  - wait
         //!
         //! @param path path to application to be spawned
         //! @param arguments 0..N arguments that is passed to the application
         //! @return exit code from the process
         int execute( std::string path, std::vector< std::string> arguments);

         //! Wait for a specific process to terminate.
         //!
         //! @return return code from process
         int wait( strong::process::id pid);

         //! Tries to terminate pids
         //!
         //! @return pids that did received the signal
         std::vector< strong::process::id> terminate( const std::vector< strong::process::id>& pids);

         //! Tries to terminate pid
         bool terminate( strong::process::id pid);

         //! Tries to shutdown the process, if it fails terminate signal will be signaled
         //!
         //! @param process to terminate
         void terminate( const Handle& process);

         namespace lifetime
         {
            struct Exit
            {
               enum class Reason : short
               {
                  core,
                  exited,
                  stopped,
                  continued,
                  signaled,
                  unknown,
               };

               strong::process::id pid;
               int status = 0;
               Reason reason = Reason::unknown;

               explicit operator bool () const;

               //! @return true if the process life has ended
               bool deceased() const;

               friend bool operator == ( strong::process::id pid, const Exit& rhs);
               friend bool operator == ( const Exit& lhs, strong::process::id pid);
               friend bool operator < ( const Exit& lhs, const Exit& rhs);

               friend std::ostream& operator << ( std::ostream& out, const Reason& value);

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( pid);
                  CASUAL_SERIALIZE( status);
                  CASUAL_SERIALIZE( reason);
               })

            };

            std::vector< Exit> ended();

            std::vector< Exit> wait( const std::vector< strong::process::id>& pids);
            std::vector< Exit> wait( const std::vector< strong::process::id>& pids, platform::time::unit timeout);

            //! Terminates and waits for the termination.
            //!
            //! @return the terminated l
            std::vector< Exit> terminate( const std::vector< strong::process::id>& pids);
            std::vector< Exit> terminate( const std::vector< strong::process::id>& pids, platform::time::unit timeout);

         } // lifetime

         namespace children
         {
            //! Terminate all children that @p pids dictates.
            //! When a child is terminated callback is called
            //!
            //! @param callback the callback object
            //! @param pids to terminate
            template< typename C>
            void terminate( C&& callback, std::vector< strong::process::id> pids)
            {
               for( auto& death : lifetime::terminate( std::move( pids)))
               {
                  callback( death);
               }
            }


         } // children

      } // process

      struct Process 
      {
         Process() = default;
         Process( const std::string& path, std::vector< std::string> arguments);
         inline Process( const std::string& path) : Process( path, {}) {}
         ~Process();
         
         Process( Process&&) noexcept = default;
         Process& operator = ( Process&&) noexcept = default;

         inline const process::Handle& handle() const noexcept { return m_handle;}
         void handle( const process::Handle& handle);

         // for logging only
         CASUAL_LOG_SERIALIZE(
         {
            CASUAL_SERIALIZE_NAME( m_handle, "handle");
         })

      private:
         process::Handle m_handle;
      };
   } // common
} // casual



