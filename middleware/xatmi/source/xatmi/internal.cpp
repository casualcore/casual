//!
//! casual 
//!

#include "xatmi/internal.h"

#include "common/service/call/context.h"

namespace casual
{
   namespace xatmi
   {
      namespace internal
      {
         namespace error
         {
            namespace local
            {
               namespace
               {
                  int value = 0;

               } // <unnamed>
            } // local

            void set( int value)
            {
               local::value = value;
            }

            int get()
            {
               return local::value;
            }
         } // tperrno

         namespace user
         {
            namespace code
            {
               void set( long value)
               {
                  casual::common::service::call::Context::instance().user_code( value);
               }
               long get()
               {
                  return casual::common::service::call::Context::instance().user_code();
               }
            } // code
         } // user
      } // internal
   } // xatmi
} // casual
