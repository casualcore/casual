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
                  common::error::code::xatmi value = common::error::code::xatmi::ok;

               } // <unnamed>
            } // local

            void set( common::error::code::xatmi value)
            {
               local::value = value;
            }

            common::error::code::xatmi get()
            {
               return local::value;
            }

            void clear()
            {
               local::value = common::error::code::xatmi::ok;
            }
         } // error

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
            error::local::value = common::error::code::xatmi::ok;
            user::code::local::value = 0;
         }

      } // internal
   } // xatmi
} // casual
