//
// field.h
//
//  Created on: 3 nov 2013
//      Author: Kristone
//

#ifndef CASUAL_BUFFER_FIELD_H
#define CASUAL_BUFFER_FIELD_H


/* used as type with tpalloc */
#define CASUAL_FIELD "CFIELD"


/* success */
#define CASUAL_FIELD_SUCCESS 0
/* the provided buffer is invalid (application/logic error) */
#define CASUAL_FIELD_INVALID_HANDLE 1
/* some argument is invalid (application/logic error) */
#define CASUAL_FIELD_INVALID_ARGUMENT 2
/* out of memory (system/runtime error) */
#define CASUAL_FIELD_OUT_OF_MEMORY 4
/* something does not exist in (perhaps normal behavior) */
#define CASUAL_FIELD_OUT_OF_BOUNDS 8
/* internal casual defect */
#define CASUAL_FIELD_INTERNAL_FAILURE 128


#define CASUAL_FIELD_NO_ID 0
#define CASUAL_FIELD_NO_TYPE 0

#define CASUAL_FIELD_SHORT 1
#define CASUAL_FIELD_LONG 2
#define CASUAL_FIELD_CHAR 3
#define CASUAL_FIELD_FLOAT 4
#define CASUAL_FIELD_DOUBLE 5
#define CASUAL_FIELD_STRING 6
#define CASUAL_FIELD_BINARY 7



/* should this be here? */
//#define CASUAL_FIELD_TYPE_BASE 0x8000
#define CASUAL_FIELD_TYPE_BASE 0x2000000


#ifdef __cplusplus
extern "C" {
#endif

/* returns the corresponding text for every return-code */
const char* casual_field_description( int code);

/*
   Writer functions
*/

/* value is encoded and added to buffer */
int casual_field_add_char(    char** buffer, long id, char value);
/* value is encoded and added to buffer */
int casual_field_add_short(   char** buffer, long id, short value);
/* value is encoded and added to buffer */
int casual_field_add_long(    char** buffer, long id, long value);
/* value is encoded and added to buffer */
int casual_field_add_float(   char** buffer, long id, float value);
/* value is encoded and added to buffer */
int casual_field_add_double(  char** buffer, long id, double value);
/* value is null-terminated added to buffer */
int casual_field_add_string(  char** buffer, long id, const char* value);
/* value is added to buffer */
int casual_field_add_binary(  char** buffer, long id, const char* value, long count);
/* value is (perhaps encoded and) added to buffer where count is relevant only for CASUAL_FIELD_BINARY */
int casual_field_add_value(   char** buffer, long id, const void* value, long count);

/* an empty value is added to buffer */
int casual_field_add_empty(   char** buffer, long id);



/*
   Reader functions
*/

/* data is decoded and stored into value */
int casual_field_get_char(    const char* buffer, long id, long index, char* value);
/* data is decoded and stored into value */
int casual_field_get_short(   const char* buffer, long id, long index, short* value);
/* data is decoded and stored into value */
int casual_field_get_long(    const char* buffer, long id, long index, long* value);
/* data is decoded and stored into value */
int casual_field_get_float(   const char* buffer, long id, long index, float* value);
/* data is decoded and stored into value */
int casual_field_get_double(  const char* buffer, long id, long index, double* value);
/* value points to internal (null-terminated) buffer */
int casual_field_get_string(  const char* buffer, long id, long index, const char** value);
/* value points to internal buffer and size is decoded and stored into count */
int casual_field_get_binary(  const char* buffer, long id, long index, const char** value, long* count);
/* data is (perhaps decoded and) copied into value if (any) count is large enough and (perhaps) updates count */
int casual_field_get_value(   const char* buffer, long id, long index, void* value, long* count);


/*
   Update functions (no values are automatically added)
*/

/* value is encoded and updated in buffer */
int casual_field_set_char(    char** buffer, long id, long index, char value);
/* value is encoded and updated in buffer */
int casual_field_set_short(   char** buffer, long id, long index, short value);
/* value is encoded and updated in buffer */
int casual_field_set_long(    char** buffer, long id, long index, long value);
/* value is encoded and updated in buffer */
int casual_field_set_float(   char** buffer, long id, long index, float value);
/* value is encoded and updated in buffer */
int casual_field_set_double(  char** buffer, long id, long index, double value);
/* value is encoded and updated in buffer */
int casual_field_set_string(  char** buffer, long id, long index, const char* value);
/* value is encoded and updated in buffer */
int casual_field_set_binary(  char** buffer, long id, long index, const char* value, long count);
/* value is (perhaps encoded and) updated in buffer where count is relevant only for CASUAL_FIELD_BINARY */
int casual_field_set_value(   char** buffer, long id, long index, const void* value, long count);

/* value is (perhaps encoded and) updated in - or added to the buffer (perhaps along with some magic empty values) */
int casual_field_put_value(   char** buffer, long id, long index, const void* value, long count);


/*
    Static functions
*/

/* get the textual name from id from repository-table */
int casual_field_name_of_id( long id, const char** name);
/* get the id from textual name from repository-table */
int casual_field_id_of_name( const char* name, long* id);
/* get the type from id */
int casual_field_type_of_id( long id, int* type);
/* get the name from type */
int casual_field_name_of_type( int type, const char** name);
/* get the type from name */
int casual_field_type_of_name( const char* name, int* type);
/* get the host-size of a pod (actually an internal helper function) */
int casual_field_plain_type_host_size( int type, long* count);


/* get allocated - and used bytes */
int casual_field_explore_buffer( const char* buffer, long* size, long* used);
/* get (host) size of any value if existing */
int casual_field_explore_value( const char* buffer, long id, long index, long* count);


/* get the total occurrences of id in buffer */
int casual_field_occurrences_of_id( const char* buffer, long id, long* occurrences);
/* get the total occurrences in buffer */
int casual_field_occurrences_in_buffer( const char* buffer, long* occurrences);


/* remove all content */
int casual_field_remove_all( char* buffer);

/* removes all occurrences of supplied id */
int casual_field_remove_id( char* buffer, long id);

/* removes supplied occurrence of supplied id and (logically) collapses possible sequential occurrences */
int casual_field_remove_occurrence( char* buffer, long id, long index);

/* gives a "handle" to the next (or first if id is CASUAL_FIELD_NO_ID) occurrence in a buffer */
int casual_field_next( const char* buffer, long* id, long* index);

/* copies content from source- to target-buffer */
int casual_field_copy_buffer( char** target, const char* source);

/* serialize (unmarshal) a memory storage to a buffer with appropriate size */
int casual_field_copy_memory( char** target, const void* source, long count);



/* prints the buffer to standard output */
int casual_field_print( const char* buffer);

/* experimental */
int casual_field_match( const char* buffer, const char* expression, int* match);
/* experimental */
int casual_field_make_expression( const char* expression, const void** regex);
/* experimental */
int casual_field_match_expression( const char* buffer, const void* regex, int* match);
/* experimental */
int casual_field_free_expression( const void* regex);

#ifdef __cplusplus
}
#endif

#endif /* CASUAL_BUFFER_FIELD_H */
