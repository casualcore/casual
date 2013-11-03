/*
 *
 * casual_order_buffer.h
 *
 * Created on: Oct 18, 2013
 *    Author: Kristone
 *
 */


#ifndef CASUAL_ORDER_BUFFER_H_
#define CASUAL_ORDER_BUFFER_H_



#include <stdbool.h>


/* used as type with tpalloc */
#define CASUAL_ORDER "CORDER"


/* success */
#define CASUAL_ORDER_SUCCESS 0
/* reallocation needed */
#define CASUAL_ORDER_NO_SPACE 1
/* detection of possible producer/consumer mismatch */
#define CASUAL_ORDER_NO_PLACE 2
/* internal casual defect */
#define CASUAL_ORDER_INTERNAL_FAILURE 3


#ifdef __cplusplus
extern "C" {
#endif

const char* CasualOrderDescription( int code);


int CasualOrderAddPrepare( char* buffer);

#ifdef __bool_true_false_are_defined
int CasualOrderAddBool(    char* buffer, bool value);
#endif
int CasualOrderAddChar(    char* buffer, char value);
int CasualOrderAddShort(   char* buffer, short value);
int CasualOrderAddLong(    char* buffer, long value);
int CasualOrderAddFloat(   char* buffer, float value);
int CasualOrderAddDouble(  char* buffer, double value);
int CasualOrderAddString(  char* buffer, const char* value);
int CasualOrderAddBinary(  char* buffer, const char* value, long size);

int CasualOrderGetPrepare( char* buffer);

#ifdef __bool_true_false_are_defined
int CasualOrderGetBool(    char* buffer, bool* value);
#endif
int CasualOrderGetChar(    char* buffer, char* value);
int CasualOrderGetShort(   char* buffer, short* value);
int CasualOrderGetLong(    char* buffer, long* value);
int CasualOrderGetFloat(   char* buffer, float* value);
int CasualOrderGetDouble(  char* buffer, double* value);
int CasualOrderGetString(  char* buffer, const char** value);
int CasualOrderGetBinary(  char* buffer, const char** value, long* size);

int CasualOrderCopyBuffer( char* target, const char* source);

int CasualOrderUsed( const char* buffer, long* length);


#ifdef __cplusplus
}
#endif


#endif /* CASUAL_ORDER_BUFFER_H_ */
