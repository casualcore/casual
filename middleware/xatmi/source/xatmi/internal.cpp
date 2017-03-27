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
               namespace local
               {
                  namespace
                  {
                     long value = 0;

                  } // <unnamed>
               } // local

               void set( long value)
               {
                  local::value = value;
               }
               long get()
               {
                  return local::value;
               }
            } // code
         } // user

         void clear()
         {
            error::local::value = 0;
            user::code::local::value = 0;
         }

      } // internal
   } // xatmi
} // casual
