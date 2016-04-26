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


   /*
    * The buffer auto-resizes (if used with casual_string-functions), so we do not need to give it a size
    */

   length = 0;

   buffer = tpalloc( CASUAL_STRING, NULL, length);

   casual_string_write( &buffer, "Hello Casual");

   tpcall( "some_service", buffer, length, &buffer, &length, TPSIGRSTRT);

   printf( "result %s", buffer);

   tpfree( buffer);

   return 0;
}




