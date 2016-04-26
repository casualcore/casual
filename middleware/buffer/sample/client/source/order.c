/*
 * order.c
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

#include <xatmi.h>
#include <buffer/order.h>


int call_with_order()
{
   char* buffer;
   long length;

   const char* id;
   long count;

   const char* name;
   double cost;

   id = "123";
   count = 2;


   /*
    * The buffer auto-resizes, so we do not need to give it a size
    */

   length = 0;

   buffer = tpalloc( CASUAL_ORDER, NULL, length);

   casual_order_add_string( &buffer, id);
   casual_order_add_long( &buffer, count);


   tpcall( "some_service", buffer, length, &buffer, &length, TPSIGRSTRT);

   casual_order_get_string( buffer, &name);
   casual_order_get_double( buffer, &cost);

   printf( "name %s", name);
   printf( "cost %f", cost);


   tpfree( buffer);

   return 0;
}



