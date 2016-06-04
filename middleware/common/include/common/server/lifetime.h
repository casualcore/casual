//!
//! shutdown.h
//!
//! Created on: Oct 27, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_SERVER_LIFETIME_H_
#define CASUAL_COMMON_SERVER_LIFETIME_H_

#include "common/process.h"


#include <chrono>

namespace casual
{
   namespace common
   {
      namespace server
      {
         namespace lifetime
         {

            namespace soft
            {
               std::vector< process::lifetime::Exit> shutdown( const std::vector< process::Handle>& servers, std::chrono::microseconds timeout);
            } // soft

            namespace hard
            {
               std::vector< process::lifetime::Exit> shutdown( const std::vector< process::Handle>& servers, std::chrono::microseconds timeout);
            } // hard





            //!
            //! Terminate all children that @p pids dictates.
            //! When a child is terminated state is updated
            //!
            //! @param state the state object
            //! @param pids to terminate
            //!
            template< typename C>
            void shutdown( C&& callback, const std::vector< process::Handle>& servers, std::vector< platform::pid::type> executables, std::chrono::microseconds timeout)
            {
               for( auto& death : process::lifetime::terminate( std::move( executables), timeout))
               {
                  callback( death);
               }

               for( auto& death : hard::shutdown( servers, timeout))
               {
                  callback( death);
               }
            }


         } // lifetime

      } // server

   } // common



} // casual

#endif // SHUTDOWN_H_
