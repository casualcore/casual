//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_XATMI_INCLUDE_XATMI_INTERNAL_H_
#define CASUAL_MIDDLEWARE_XATMI_INCLUDE_XATMI_INTERNAL_H_

#include "common/exception/xatmi.h"

namespace casual
{
   namespace xatmi
   {
      namespace internal
      {
         void clear();

         namespace error
         {
            void clear();
            void set( common::code::xatmi value);
            common::code::xatmi get();

            template< typename T>
            int wrap( T&& task)
            {
               try
               {
                  error::clear();
                  task();

                  return 0;
               }
               catch( ...)
               {
                  error::set( casual::common::exception::xatmi::handle());
               }
               return -1;
            }
         } // error

         namespace user
         {
            namespace code
            {
               void set( long value);
               long get();
            } // code
         } // user
      } // internal
   } // xatmi
} // casual

#endif // CASUAL_MIDDLEWARE_XATMI_INCLUDE_XATMI_INTERNAL_H_
