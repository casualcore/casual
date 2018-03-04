//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_THREAD_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_THREAD_H_

#include <thread>

namespace casual
{
   namespace common
   {
      namespace thread
      {
         namespace native
         {
            using type = decltype( std::thread().native_handle());

            //!
            //! @return the native handler for current thread
            //!
            type current();

         } // native

      } // thread
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_THREAD_H_
