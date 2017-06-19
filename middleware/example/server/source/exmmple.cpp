//!
//! casual 
//!

#include "xatmi.h"


#include <array>

namespace casual
{

   namespace example
   {
      namespace server
      {

         extern "C"
         {
            void casual_example_echo( TPSVCINFO* info)
            {
               tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
            }

            void casual_example_conversation( TPSVCINFO* info)
            {
               tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
            }

            void casual_example_sink( TPSVCINFO* info)
            {
               tpreturn( TPSUCCESS, 0, nullptr, 0, 0);
            }

            void casual_example_rollback( TPSVCINFO* info)
            {
               tpreturn( TPFAIL, 0, info->data, info->len, 0);
            }

         }
      } // server
   } // example
} // casual
