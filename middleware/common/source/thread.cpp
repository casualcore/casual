//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/thread.h"


#include <pthread.h>

namespace casual
{
   namespace common
   {
      namespace thread
      {
         namespace native
         {
            type current()
            {
               return pthread_self();
            }

         } // native


      } // thread

   } // common


} // casual
