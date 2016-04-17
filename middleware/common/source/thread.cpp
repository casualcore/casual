//!
//! casual 
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
