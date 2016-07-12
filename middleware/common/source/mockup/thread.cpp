//!
//! casual 
//!

#include "common/mockup/thread.h"
#include "common/mockup/log.h"


#include "common/error.h"
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
               common::error::handler();
            }
         }


      } // mockup
   } // common
} // casual
