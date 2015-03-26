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
/* reallocation needed (normal behavior) */
#define CASUAL_FIELD_NO_SPACE 1
/* id does not exist in buffer (perhaps normal behavior) */
#define CASUAL_FIELD_NO_OCCURRENCE 2
/* id unknown by the system (system/runtime failure) */
#define CASUAL_FIELD_UNKNOWN_ID 3
/* the provided buffer is invalid (application/logic error) */
#define CASUAL_FIELD_INVALID_BUFFER 4
/* id does not represent supplied type (application/logic error) */
#define CASUAL_FIELD_INVALID_ID 5
/* type does not represent any known type (application/logic error) */
#define CASUAL_FIELD_INVALID_TYPE 6
/* some argument is invalid (application/logic error) */
#define CASUAL_FIELD_INVALID_ARGUMENT 7
/* general system (os) failure (system/runtime failure) */
#define CASUAL_FIELD_SYSTEM_FAILURE 8
/* internal casual defect */
#define CASUAL_FIELD_INTERNAL_FAILURE 9


#define CASUAL_FIELD_NO_ID 0
#define CASUAL_FIELD_NO_TYPE 0

/*
#define CASUAL_FIELD_SHORT 0
#define CASUAL_FIELD_LONG 1
#define CASUAL_FIELD_CHAR 2
#define CASUAL_FIELD_FLOAT 3
#define CASUAL_FIELD_DOUBLE 4
#define CASUAL_FIELD_STRING 5
#define CASUAL_FIELD_BINARY 6
*/

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
const char* CasualFieldDescription( int code);

/*
   Writer functions
*/

/* value is encoded and added to buffer */
int CasualFieldAddChar(    char* buffer, long id, char value);
/* value is encoded and added to buffer */
int CasualFieldAddShort(   char* buffer, long id, short value);
/* value is encoded and added to buffer */
int CasualFieldAddLong(    char* buffer, long id, long value);
/* value is encoded and added to buffer */
int CasualFieldAddFloat(   char* buffer, long id, float value);
/* value is encoded and added to buffer */
int CasualFieldAddDouble(  char* buffer, long id, double value);
/* value is null-terminated added to buffer */
int CasualFieldAddString(  char* buffer, long id, const char* value);
/* value is added to buffer */
int CasualFieldAddBinary(  char* buffer, long id, const char* value, long count);
/* value is (perhaps encoded and) added to buffer where count is relevant only for CASUAL_FIELD_BINARY */
int CasualFieldAddValue(   char* buffer, long id, const void* value, long count);

/* an empty value is added to buffer */
int CasualFieldAddEmpty(   char* buffer, long id);



/*
   Reader functions
*/

/* data is decoded and stored into value */
int CasualFieldGetChar(    const char* buffer, long id, long index, char* value);
/* data is decoded and stored into value */
int CasualFieldGetShort(   const char* buffer, long id, long index, short* value);
/* data is decoded and stored into value */
int CasualFieldGetLong(    const char* buffer, long id, long index, long* value);
/* data is decoded and stored into value */
int CasualFieldGetFloat(   const char* buffer, long id, long index, float* value);
/* data is decoded and stored into value */
int CasualFieldGetDouble(  const char* buffer, long id, long index, double* value);
/* value points to internal (null-terminated) buffer */
int CasualFieldGetString(  const char* buffer, long id, long index, const char** value);
/* value points to internal buffer and size is decoded and stored into count */
int CasualFieldGetBinary(  const char* buffer, long id, long index, const char** value, long* count);
/* data is (perhaps decoded and) copied into value if (any) count is large enough and (perhaps) updates count */
int CasualFieldGetValue(   const char* buffer, long id, long index, void* value, long* count);


/*
   Update functions (no values are automatically added)
*/

/* value is encoded and updated in buffer */
int CasualFieldUpdateChar(    char* buffer, long id, long index, char value);
/* value is encoded and updated in buffer */
int CasualFieldUpdateShort(   char* buffer, long id, long index, short value);
/* value is encoded and updated in buffer */
int CasualFieldUpdateLong(    char* buffer, long id, long index, long value);
/* value is encoded and updated in buffer */
int CasualFieldUpdateFloat(   char* buffer, long id, long index, float value);
/* value is encoded and updated in buffer */
int CasualFieldUpdateDouble(  char* buffer, long id, long index, double value);
/* value is encoded and updated in buffer */
int CasualFieldUpdateString(  char* buffer, long id, long index, const char* value);
/* value is encoded and updated in buffer */
int CasualFieldUpdateBinary(  char* buffer, long id, long index, const char* value, long count);
/* value is (perhaps encoded and) updated in buffer where count is relevant only for CASUAL_FIELD_BINARY */
int CasualFieldUpdateValue(   char* buffer, long id, long index, const void* value, long count);

/* value is (perhaps encoded and) updated in - or added to the buffer (perhaps along with some magic empty values) */
int CasualFieldChangeValue(   char* buffer, long id, long index, const void* value, long count);


/*
    Static functions
*/

/* get the textual name from id from repository-table */
int CasualFieldNameOfId( long id, const char** name);
/* get the id from textual name from repository-table */
int CasualFieldIdOfName( const char* name, long* id);
/* get the type from id */
int CasualFieldTypeOfId( long id, int* type);
/* get the name from type */
int CasualFieldNameOfType( int type, const char** name);
/* get the type from name */
int CasualFieldTypeOfName( const char* name, int* type);
/* get the host-size of a pod (actually an internal helper function) */
int CasualFieldPlainTypeHostSize( int type, long* count);


/* get allocated - and used bytes */
int CasualFieldExploreBuffer( const char* buffer, long* size, long* used);
/* get (host) size of any value if existing */
int CasualFieldExploreValue( const char* buffer, long id, long index, long* count);


/* get the total occurrences of id in buffer */
int CasualFieldOccurrencesOfId( const char* buffer, long id, long* occurrences);
/* get the total occurrences in buffer */
int CasualFieldOccurrencesInBuffer( const char* buffer, long* occurrences);


/* remove all content */
int CasualFieldRemoveAll( char* buffer);

/* removes all occurrences of supplied id */
int CasualFieldRemoveId( char* buffer, long id);

/* removes supplied occurrence of supplied id and (logically) collapses possible sequential occurrences */
int CasualFieldRemoveOccurrence( char* buffer, long id, long index);

/* copies content from source- to target-buffer */
int CasualFieldCopyBuffer( char* target, const char* source);


/* gives a "handle" to the next (or first if id is CASUAL_FIELD_NO_ID) occurrence in a buffer */
int CasualFieldNext( const char* buffer, long* id, long* index);


/* prints the buffer to standard output */
int CasualFieldPrint( const char* buffer);

/* experimental */
int CasualFieldMatch( const char* buffer, const char* expression, int* match);



#ifdef __cplusplus
}
#endif

#endif /* CASUAL_BUFFER_FIELD_H */
