//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
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
                  common::code::xatmi value = common::code::xatmi::ok;

               } // <unnamed>
            } // local

            void set( common::code::xatmi value)
            {
               local::value = value;
            }

            common::code::xatmi get()
            {
               return local::value;
            }

            void clear()
            {
               local::value = common::code::xatmi::ok;
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
            error::local::value = common::code::xatmi::ok;
            user::code::local::value = 0;
         }

      } // internal
   } // xatmi
} // casual
