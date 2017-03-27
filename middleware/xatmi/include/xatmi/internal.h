//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_XATMI_INCLUDE_XATMI_INTERNAL_H_
#define CASUAL_MIDDLEWARE_XATMI_INCLUDE_XATMI_INTERNAL_H_

#include "common/error.h"

namespace casual
{
   namespace xatmi
   {
      namespace internal
      {
         void clear();

         namespace error
         {
            void set( int value);
            int get();


            template< typename T>
            int wrap( T&& task)
            {
               try
               {
                  error::set( 0);
                  task();
               }
               catch( ...)
               {
                  error::set( casual::common::error::handler());
               }
               return error::get() == 0 ? 0 : -1;
            }

         } // tperrno

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
