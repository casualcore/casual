//
// casual_field_buffer.h
//
//  Created on: 3 nov 2013
//      Author: Kristone
//

#ifndef CASUAL_FIELD_BUFFER_H
#define CASUAL_FIELD_BUFFER_H

#include <stdbool.h>


/* used as type with tpalloc */
#define CASUAL_FIELD "CFIELD"



/* success */
#define CASUAL_FIELD_SUCCESS 0
/* reallocation needed (normal behaviour) */
#define CASUAL_FIELD_NO_SPACE 1
/* id does not exist in buffer (perhaps normal behaviour) */
#define CASUAL_FIELD_NO_OCCURRENCE 2
/* id unknown by the system (system/runtime failure) */
#define CASUAL_FIELD_UNKNOWN_ID 3
/* id does not represent supplied type (application/logic error*/
#define CASUAL_FIELD_INVALID_ID 4
/* internal casual defect */
#define CASUAL_FIELD_INTERNAL_FAILURE 5


#ifdef __bool_true_false_are_defined
#define CASUAL_FIELD_BOOL 1
#endif
#define CASUAL_FIELD_CHAR 2
#define CASUAL_FIELD_SHORT 3
#define CASUAL_FIELD_LONG 4
#define CASUAL_FIELD_FLOAT 5
#define CASUAL_FIELD_DOUBLE 6
#define CASUAL_FIELD_STRING 7
#define CASUAL_FIELD_BINARY 8



const char* CasualFieldDescription( int code);

#ifdef __bool_true_false_are_defined
int CasualFieldAddBool(    char* buffer, long id, bool value);
#endif
int CasualFieldAddChar(    char* buffer, long id, char value);
int CasualFieldAddShort(   char* buffer, long id, short value);
int CasualFieldAddLong(    char* buffer, long id, long value);
int CasualFieldAddFloat(   char* buffer, long id, float value);
int CasualFieldAddDouble(  char* buffer, long id, double value);
int CasualFieldAddString(  char* buffer, long id, const char* value);
int CasualFieldAddBinary(  char* buffer, long id, const char* value, long size);

int CasualFieldAddData(    char* buffer, long id, void* value, long size);

#ifdef __bool_true_false_are_defined
int CasualFieldGetBool(    const char* buffer, long id, long index, bool* value);
#endif
int CasualFieldGetChar(    const char* buffer, long id, long index, char* value);
int CasualFieldGetShort(   const char* buffer, long id, long index, short* value);
int CasualFieldGetLong(    const char* buffer, long id, long index, long* value);
int CasualFieldGetFloat(   const char* buffer, long id, long index, float* value);
int CasualFieldGetDouble(  const char* buffer, long id, long index, double* value);
int CasualFieldGetString(  const char* buffer, long id, long index, const char** value);
int CasualFieldGetBinary(  const char* buffer, long id, long index, const char** value, long* size);

int CasualFieldGetData(    const char* buffer, long id, long index, const void** value, long* size);


int CasualFieldExploreBuffer( const char* buffer, long* size, long* used);
int CasualFieldExploreId( const char* name, long* id);
int CasualFieldExploreName( long id, const char** name);
int CasualFieldExploreType( long id, int* type);



/* remove all content */
int CasualFieldRemoveAll( char* buffer);

/* removes all occurrences with supplied id but more space will not be available */
int CasualFieldRemoveId( char* buffer, long id);

/* removes supplied occurrence with supplied id and collapses possible sequential occurrences but more space will not be available */
int CasualFieldRemoveOccurrence( char* buffer, long id, long occurrence);

/* copies content from 'source' to 'target' */
int CasualFieldCopyBuffer( char* target, const char* source);






/*
   Iterator-operations
*/
/*
long CasualFieldNumberOfIds( const char* buffer);
long CasualFieldNumberOfOccurrences( const char* buffer, long id);
int CasualFieldIterateFirst( const char* buffer, long* id, long* index);
int CasualFieldIterateFirst( const char* buffer, long* id);
int CasualFieldIterateNext( const char* buffer, long* id);
int CasualFieldIterateNext( const char* buffer, long* id, long* index);
*/




#endif /* CASUAL_FIELD_BUFFER_H_ */
