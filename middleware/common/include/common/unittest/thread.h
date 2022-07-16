//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include <thread>

#include "common/unittest/log.h"

namespace casual
{
   namespace common
   {
      namespace unittest
      {
         //! as std::thread, but sends signal terminate on destruction
         //! and joins the thread
         struct Thread
         {
            Thread() noexcept;

            template< typename... Args>
            Thread( Args&&... args)
               : m_thread{ std::forward< Args>( args)...}
            {
               log::line( log, "mockup::Thread ctor - thread: ", m_thread.get_id());
            }

            Thread( Thread&&) noexcept;
            Thread& operator = ( Thread&&) noexcept;

            ~Thread();

         private:
            std::thread m_thread;
         };
      } // unittest
   } // common
} // casual


