/*
 * field.c
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
#include <buffer/field.h>

/*
 * Just for this sample (see casual_field_make_header)
 */
#define FLD_ID       (CASUAL_FIELD_STRING * CASUAL_FIELD_TYPE_BASE + 1)
#define FLD_COUNT    (CASUAL_FIELD_LONG   * CASUAL_FIELD_TYPE_BASE + 1)
#define FLD_NAME     (CASUAL_FIELD_STRING * CASUAL_FIELD_TYPE_BASE + 2)
#define FLD_COST     (CASUAL_FIELD_DOUBLE * CASUAL_FIELD_TYPE_BASE + 1)


int call_with_field()
{
   char* buffer;
   long length;

   const char* name;
   double cost;


   /*
    * The buffer auto-resizes, so we do not need to give it a size
    */

   length = 0;

   buffer = tpalloc( CASUAL_FIELD, NULL, length);

   casual_field_add_string( &buffer, FLD_ID, "123");
   casual_field_add_long( &buffer, FLD_COUNT, 2);


   tpcall( "some_service", buffer, length, &buffer, &length, TPSIGRSTRT);

   casual_field_get_string( buffer, FLD_NAME, 0, &name);
   casual_field_get_double( buffer, FLD_COST, 0, &cost);

   printf( "name %s", name);
   printf( "cost %f", cost);


   tpfree( buffer);

   return 0;
}
