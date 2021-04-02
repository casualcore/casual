//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest/thread.h"

#include "common/exception/handle.h"
#include "common/signal.h"

namespace casual
{
   namespace common
   {
      namespace unittest
      {

         Thread::Thread() noexcept = default;
         Thread::Thread( Thread&&) noexcept = default;

         Thread& Thread::operator = ( Thread&&) noexcept = default;

         Thread::~Thread()
         {
            try
            {
               log::line( log, "mockup::Thread dtor - thread: ", m_thread.get_id());

               if( m_thread.joinable())
               {
                  signal::thread::send( m_thread, code::signal::user);
                  m_thread.join();
               }
            }
            catch( ...)
            {
               log::line( log::category::error, exception::capture(), " Thread::~Thread");
            }
         }


      } // mockup
   } // common
} // casual
