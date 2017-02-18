/*
 * string.c
 *
 *  Created on: Apr 26, 2016
 *      Author: kristone
 */



#include <stddef.h>
#include <stdio.h>

#include <xatmi.h>
#include <buffer/string.h>


int call_with_string()
{
   char* buffer;
   long length;

   int result;

   /*
    * The buffer auto-resizes (if used with casual_string-functions), so we do not need to give it a size
    */

   length = 0;

   buffer = tpalloc( CASUAL_STRING, NULL, length);

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




