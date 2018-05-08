//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


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
               std::vector< process::lifetime::Exit> shutdown( const std::vector< process::Handle>& servers, common::platform::time::unit timeout);
            } // soft

            namespace hard
            {
               std::vector< process::lifetime::Exit> shutdown( const std::vector< process::Handle>& servers, common::platform::time::unit timeout);
            } // hard





            //!
            //! Terminate all children that @p pids dictates.
            //! When a child is terminated state is updated
            //!
            //! @param state the state object
            //! @param pids to terminate
            //!
            template< typename C>
            void shutdown( C&& callback, const std::vector< process::Handle>& servers, std::vector< strong::process::id> executables, common::platform::time::unit timeout)
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


