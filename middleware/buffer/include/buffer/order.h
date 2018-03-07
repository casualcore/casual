//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_ORDER_BUFFER_H_
#define CASUAL_ORDER_BUFFER_H_

#include <stdbool.h>


/* used as type with tpalloc */
#define CASUAL_ORDER "CORDER"


/* success */
#define CASUAL_ORDER_SUCCESS 0
/* the provided buffer is invalid (application/logic error) */
#define CASUAL_ORDER_INVALID_HANDLE 1
/* some argument is invalid (application/logic error) */
#define CASUAL_ORDER_INVALID_ARGUMENT 2
/* out of memory (system/runtime error) */
#define CASUAL_ORDER_OUT_OF_MEMORY 4
/* detection of possible producer/consumer mismatch */
#define CASUAL_ORDER_OUT_OF_BOUNDS 8
/* internal casual defect */
#define CASUAL_ORDER_INTERNAL_FAILURE 128




#ifdef __cplusplus
extern "C" {
#endif

const char* casual_order_description( int code);


/* Get allocated - and used bytes */
int casual_order_explore_buffer( const char* buffer, long* reserved, long* utilized, long* consumed);

/* Reset the append-cursor (only needed if/when recycling the buffer) */
int casual_order_add_prepare( const char* buffer);

#ifdef __bool_true_false_are_defined
int casual_order_add_bool(    char** buffer, bool value);
#endif
int casual_order_add_char(    char** buffer, char value);
int casual_order_add_short(   char** buffer, short value);
int casual_order_add_long(    char** buffer, long value);
int casual_order_add_float(   char** buffer, float value);
int casual_order_add_double(  char** buffer, double value);
int casual_order_add_string(  char** buffer, const char* value);
int casual_order_add_binary(  char** buffer, const char* data, long size);

/* Reset the select-cursor (only needed if/when recycling the buffer) */
int casual_order_get_prepare( const char* buffer);

#ifdef __bool_true_false_are_defined
int casual_order_get_bool(    const char* buffer, bool* value);
#endif
int casual_order_get_char(    const char* buffer, char* value);
int casual_order_get_short(   const char* buffer, short* value);
int casual_order_get_long(    const char* buffer, long* value);
int casual_order_get_float(   const char* buffer, float* value);
int casual_order_get_double(  const char* buffer, double* value);
int casual_order_get_string(  const char* buffer, const char** value);
int casual_order_get_binary(  const char* buffer, const char** data, long* size);


#ifdef __cplusplus
}
#endif

#endif /* CASUAL_ORDER_BUFFER_H_ */
