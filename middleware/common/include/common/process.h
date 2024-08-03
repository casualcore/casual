//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/strong/id.h"
#include "common/uuid.h"

#include "common/algorithm.h"
#include "common/environment/variable.h"

#include "common/serialize/macro.h"

#include <string>
#include <vector>
#include <chrono>
#include <filesystem>

namespace casual
{
   namespace common::process
   {

      //! @return the path of the current process
      const std::filesystem::path& path();

      //! Holds pid and ipc-queue for a given process
      struct Handle
      {
         // enable environment variable serialization
         using environment_variable_enable = void;

         Handle() = default;
         inline explicit Handle( strong::process::id pid) : pid{ pid} {}
         inline Handle( strong::process::id pid, strong::ipc::id ipc) : pid( pid),  ipc( std::move( ipc))  {}

         strong::process::id pid;

         //! unique identifier for this process
         strong::ipc::id ipc;

         inline friend bool operator == ( const Handle&, const Handle&) = default;
         inline friend auto operator <=> ( const Handle&, const Handle&) = default;

         //! extended equality
         inline friend bool operator == ( const Handle& lhs, strong::process::id rhs) noexcept { return lhs.pid == rhs;}
         inline friend bool operator == ( const Handle& lhs, const strong::ipc::id& rhs) noexcept { return lhs.ipc == rhs;}

         inline explicit operator bool() const noexcept { return pid && ipc;}

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( pid);
            CASUAL_SERIALIZE( ipc);
         )
      };

      //! concept to help define equality compare with ipc and pid
      template< typename T>
      concept compare_equal_to_handle = concepts::any_of< T, strong::ipc::id, strong::process::id, Handle> &&  requires( const Handle& a, const T& b) 
      {
         { a == b} -> std::convertible_to< bool>;
         { a != b} -> std::convertible_to< bool>;
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
      //! @throws exception::system::interrupted if a signal is received
      template< typename R, typename P>
      void sleep( std::chrono::duration< R, P> time)
      {
         sleep( std::chrono::duration_cast< platform::time::unit>( time));
      }


      //! Spawn a new application that path describes
      //!
      //! @param path path to application to be spawned
      //! @param arguments 0..N arguments that is passed to the application
      //! @return process id of the spawned process
      strong::process::id spawn( std::filesystem::path path, std::vector< std::string> arguments);


      strong::process::id spawn(
         std::filesystem::path path,
         std::vector< std::string> arguments,
         std::vector< environment::Variable> environment);

      
      namespace capture
      {
         struct Standard
         {
            std::string out;
            std::string error;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( out);
               CASUAL_SERIALIZE( error);
            )
         };
      } // capture

      struct Capture
      {
         int exit{};
         capture::Standard standard;

         inline explicit operator bool() const noexcept { return exit == 0;}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( exit);
            CASUAL_SERIALIZE( standard);
         )
      };

      //! Spawn a new application that path describes, and wait until it exit. That is
      //!  - spawn
      //!  - wait
      //!
      //! @param path path to application to be spawned
      //! @param arguments 0..N arguments that is passed to the application
      //! @return the captured output from the process
      Capture execute( std::filesystem::path path, std::vector< std::string> arguments, std::vector< environment::Variable> environment = {});

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

      //! Tries to shutdown the process, if it fails terminate signal will be signaled.
      //! @attention waits until the process dies.
      //!
      //! @param process to terminate
      void terminate( const Handle& process);

      //! Tries to shutdown the processes, if it fails terminate signal will be signaled
      //!
      //! @return pids that got the directive delivered
      std::vector< strong::process::id> terminate( const std::vector< Handle>& processes);

      namespace lifetime
      {
         namespace exit
         {
            enum class Reason : short
            {
               unknown,
               core,
               exited,
               stopped,
               continued,
               signaled,
            };

            std::string_view description( Reason value) noexcept;
            
         } // exit
         struct Exit
         {
            strong::process::id pid;
            int status = 0;
            exit::Reason reason = exit::Reason::unknown;

            explicit operator bool () const;

            //! @return true if the process life has ended
            bool deceased() const;

            friend bool operator == ( strong::process::id pid, const Exit& rhs);
            friend bool operator == ( const Exit& lhs, strong::process::id pid);
            friend bool operator < ( const Exit& lhs, const Exit& rhs);

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( pid);
               CASUAL_SERIALIZE( status);
               CASUAL_SERIALIZE( reason);
            )
         };

         std::vector< Exit> ended();

         std::vector< Exit> wait( const std::vector< strong::process::id>& pids);
         std::vector< Exit> wait( const std::vector< strong::process::id>& pids, platform::time::unit timeout);

         //! Terminates and waits for the termination.
         //!
         //! @return the terminated l
         std::vector< Exit> terminate( const std::vector< Handle>& handles);
         std::vector< Exit> terminate( const std::vector< strong::process::id>& pids);
         std::vector< Exit> terminate( const std::vector< strong::process::id>& pids, platform::time::unit timeout);

      } // lifetime

      namespace children
      {
         //! Terminate all children that @p processes dictates.
         //! When a child is terminated callback is called
         //!
         //! @param callback the callback object
         //! @param processes to terminate (pids or process::Handles)
         template< typename C, typename P>
         void terminate( C&& callback, P&& processes)
         {
            for( auto& death : lifetime::terminate( std::forward< P>( processes)))
            {
               callback( death);
            }
         }

      } // children

   } // common::process

   namespace common
   {
      struct Process : process::Handle
      {
         Process();
         Process( strong::process::id pid);
         Process( const std::filesystem::path& path, std::vector< std::string> arguments);
         Process( const std::filesystem::path& path);
         ~Process();
         
         Process( Process&&) noexcept;
         Process& operator = ( Process&&) noexcept;

         inline const process::Handle& handle() const noexcept { return *this;}
         void handle( const process::Handle& handle);

         //! clears the handle, and no terminate on destruction will be attempted.
         //! only (?) useful when detected that the actual child process has died.
         void clear();
      };

   } // common
} // casual



