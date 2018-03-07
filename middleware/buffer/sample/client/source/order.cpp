//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


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

   int result;

   id = "123";
   count = 2;


   /*
    * The buffer auto-resizes, so we do not need to give it a size
    */

   length = 0;

   buffer = tpalloc( CASUAL_ORDER, NULL, length);

   result = CASUAL_ORDER_SUCCESS;

   result |= casual_order_add_string( &buffer, id);
   result |= casual_order_add_long( &buffer, count);

   if( result)
   {
      /* something went wrong */
   }

   tpcall( "some_service", buffer, length, &buffer, &length, TPSIGRSTRT);


   result |= casual_order_get_string( buffer, &name);
   result |= casual_order_get_double( buffer, &cost);

   if( result)
   {
      /* something went wrong */
   }

   printf( "name %s", name);
   printf( "cost %f", cost);


   tpfree( buffer);

   return 0;
}



