//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MOCKUP_THREAD_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MOCKUP_THREAD_H_



#include <thread>

namespace casual
{
   namespace common
   {
      namespace mockup
      {
         //!
         //! as std::thread, but sends signal terminate on destruction
         //! and joins the thread
         //!
         struct Thread
         {
            Thread() noexcept;

            template< typename... Args>
            Thread( Args&&... args)
               : m_thread{ std::forward< Args>( args)...}
            {
            }

            Thread( Thread&&) noexcept;
            Thread& operator = ( Thread&&) noexcept;

            ~Thread();

         private:

            std::thread m_thread;
         };
      } // mockup
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MOCKUP_THREAD_H_
