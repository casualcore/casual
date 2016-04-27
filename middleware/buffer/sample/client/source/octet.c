/*
 * octet.c
 *
 *  Created on: Apr 26, 2016
 *      Author: kristone
 */



/*
 * Disclaimer: This might not be a valid C-function
 *
 * Disclaimer: A lot of error handling is missing
 */


#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <xatmi.h>
#include <buffer/octet.h>


int call_with_octet()
{
   char* buffer;
   long length;

   const char* json;

   int result;

   json = "{ 'id': 25}";

   length = strlen( json);

   buffer = tpalloc( CASUAL_OCTET, CASUAL_OCTET_JSON, 0);

   result = casual_octet_set( &buffer, json, length);

   if( result)
   {
      /* something went wrong */
   }


   tpcall( "some_service", buffer, length, &buffer, &length, TPSIGRSTRT);

   result = casual_octet_get( buffer, &json, &length);

   if( result)
   {
      /* something went wrong */
   }


   tpfree( buffer);

   return 0;
}


