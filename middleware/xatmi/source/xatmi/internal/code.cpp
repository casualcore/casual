//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/xatmi/internal/code.h"

#include "common/service/call/context.h"
#include "common/exception/handle.h"
#include "common/code/category.h"

namespace casual
{
   namespace xatmi
   {
      namespace internal
      {


         namespace exception
         {
            common::code::xatmi code( std::error_code code) noexcept
            {
               if( common::code::is::category< common::code::xatmi>( code))
                  return static_cast< common::code::xatmi>( code.value());

               return common::code::xatmi::system;
            }

            common::code::xatmi code() noexcept
            {
               return exception::code( common::exception::code());
            }
         } // exception

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

extern "C" 
{
   void casual_set_tperrno( int code)
   {
      casual::xatmi::internal::error::set( casual::common::code::xatmi( code));
   }
}

