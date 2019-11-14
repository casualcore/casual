//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "xatmi.h"
#include "casual/buffer/string.h"

#include <cstddef>
#include <cstdio>


int call_with_string()
{
   char* buffer;
   long length;

   int result;

   /*
    * The buffer auto-resizes (if used with casual_string-functions), so we do not need to give it a size
    */

   length = 0;

   buffer = tpalloc( CASUAL_STRING, nullptr, length);

   result = casual_string_set( &buffer, "Hello Casual");

   if( result)
   {
      /* something went wrong */
   }



   tpcall( "some_service", buffer, length, &buffer, &length, TPSIGRSTRT);

   printf( "result %s", buffer);

   tpfree( buffer);

   return 0;
}




