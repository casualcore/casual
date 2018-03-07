//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/mockup/thread.h"
#include "common/mockup/log.h"

#include "common/exception/handle.h"
#include "common/signal.h"

namespace casual
{
   namespace common
   {
      namespace mockup
      {

         Thread::Thread() noexcept = default;
         Thread::Thread( Thread&&) noexcept = default;

         Thread& Thread::operator = ( Thread&&) noexcept = default;

         Thread::~Thread()
         {
            try
            {
               log << "mockup::Thread dtor - thread: " << m_thread.get_id() << '\n';

               if( m_thread.joinable())
               {
                  signal::thread::send( m_thread, signal::Type::terminate);
                  m_thread.join();
               }
            }
            catch( ...)
            {
               exception::handle();
            }
         }


      } // mockup
   } // common
} // casual
