//
// casual_field_buffer.h
//
//  Created on: 3 nov 2013
//      Author: Kristone
//

#ifndef CASUAL_FIELD_BUFFER_H
#define CASUAL_FIELD_BUFFER_H


/* used as type with tpalloc */
#define CASUAL_FIELD "CFIELD"


/* success */
#define CASUAL_FIELD_SUCCESS 0
/* the provided buffer is invalid */
#define CASUAL_FIELD_INVALID_BUFFER 1
/* reallocation needed (normal behaviour) */
#define CASUAL_FIELD_NO_SPACE 2
/* id does not exist in buffer (perhaps normal behaviour) */
#define CASUAL_FIELD_NO_OCCURRENCE 3
/* id unknown by the system (system/runtime failure) */
#define CASUAL_FIELD_UNKNOWN_ID 4
/* id does not represent supplied type (application/logic error*/
#define CASUAL_FIELD_INVALID_ID 5
/* type does not represent any known type (application/logic error*/
#define CASUAL_FIELD_INVALID_TYPE 6
/* internal casual defect */
#define CASUAL_FIELD_INTERNAL_FAILURE 9


/* should this be here? */
//#define CASUAL_FIELD_TYPE_BASE 0x8000
#define CASUAL_FIELD_TYPE_BASE 0x2000000

#define CASUAL_FIELD_NO_ID 0


#define CASUAL_FIELD_SHORT 0
#define CASUAL_FIELD_LONG 1
#define CASUAL_FIELD_CHAR 2
#define CASUAL_FIELD_FLOAT 3
#define CASUAL_FIELD_DOUBLE 4
#define CASUAL_FIELD_STRING 5
#define CASUAL_FIELD_BINARY 6


#ifdef __cplusplus
extern "C" {
#endif

/* returns the corresponding text for every return-code */
const char* CasualFieldDescription( int code);


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


/* get allocated - and used bytes */
int CasualFieldExploreBuffer( const char* buffer, long* size, long* used);

int CasualFieldAddChar(    char* buffer, long id, char value);
int CasualFieldAddShort(   char* buffer, long id, short value);
int CasualFieldAddLong(    char* buffer, long id, long value);
int CasualFieldAddFloat(   char* buffer, long id, float value);
int CasualFieldAddDouble(  char* buffer, long id, double value);
int CasualFieldAddString(  char* buffer, long id, const char* value);
int CasualFieldAddBinary(  char* buffer, long id, const char* value, long count);
int CasualFieldAddField(   char* buffer, long id, const void* value, long count);

int CasualFieldGetChar(    const char* buffer, long id, long index, char* value);
int CasualFieldGetShort(   const char* buffer, long id, long index, short* value);
int CasualFieldGetLong(    const char* buffer, long id, long index, long* value);
int CasualFieldGetFloat(   const char* buffer, long id, long index, float* value);
int CasualFieldGetDouble(  const char* buffer, long id, long index, double* value);
int CasualFieldGetString(  const char* buffer, long id, long index, const char** value);
int CasualFieldGetBinary(  const char* buffer, long id, long index, const char** value, long* count);
int CasualFieldGetField(   const char* buffer, long id, long index, const void** value, long* count);


int CasualFieldExist( const char* buffer, long id, long index);

/* remove all content */
int CasualFieldRemoveAll( char* buffer);

/* removes all occurrences with supplied id but more space will not be available */
int CasualFieldRemoveId( char* buffer, long id);

/* removes supplied occurrence with supplied id and collapses possible sequential occurrences but more space will not be available */
int CasualFieldRemoveOccurrence( char* buffer, long id, long index);

/* copies content from 'source' to 'target' */
int CasualFieldCopyBuffer( char* target, const char* source);



/* gives a "handle" to the first occurrence in a buffer */
int CasualFieldFirst( const char* buffer, long* id, long* index);
/* gives a "handle" to the next (or first) occurrence in a buffer (id and index is therefore relevant) */
int CasualFieldNext( const char* buffer, long* id, long* index);


#ifdef __cplusplus
}
#endif

#endif /* CASUAL_FIELD_BUFFER_H_ */
