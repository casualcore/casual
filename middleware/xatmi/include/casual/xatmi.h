/** 
 ** Copyright (c) 2015, The casual project
 **
 ** This software is licensed under the MIT license, https://opensource.org/licenses/MIT
 **/

#pragma once

#include "casual/xatmi/defines.h"
#include "casual/xatmi/extended.h"
#include "casual/xatmi/flag.h"
#include "casual/xatmi/code.h"


#define CASUAL_XATMI_IMPLEMENTATION 1

// NOW STARTS THE MAIN OPERATION LIST


#ifdef __cplusplus
extern "C" {
#endif
typedef void( *tpservice)( TPSVCINFO *);

extern char* tpalloc( const char* type, const char* subtype, long size); // MEMORY
extern char* tprealloc( const char* ptr, long size); // MEMORY


extern int tpcall( const char* svc, char* idata, long ilen, char** odata, long* olen, long flags); // COMMUNICATION
extern int tpacall( const char* svc, char* idata, long ilen, long flags); // COMMUNICATION
extern int tpgetrply( int* idPtr, char ** odata, long* olen, long flags); // COMMUNICATION
extern int tpcancel( int id); // COMMUNICATION
extern long tptypes( const char* buffer, char* type, char* subtype); // MEMORY
extern void tpfree( const char* ptr); // MEMORY

extern void tpreturn( int rval, long rcode, char* data, long len, long flags); // TJJ ADDED

extern int tpadvertise( const char* svcname, void(*func)(TPSVCINFO *)); // SERVER
extern int tpunadvertise( const char* svcname); // SERVER

// COMMUNICATION
extern int tpconnect( const char* svc, const char* idata, long ilen, long flags);
extern int tpsend( int id, const char* idata, long ilen, long flags, long *revent);
extern int tprecv( int id, char ** odata, long *olen, long flags, long* event);
extern int tpdiscon( int id);


extern int casual_get_tperrno(void);
extern long casual_get_tpurcode(void);

#define tperrno casual_get_tperrno()
#define tpurcode casual_get_tpurcode()


extern const char* tperrnostring( int error);

extern int tpsvrinit( int argc, char** argv);
extern void tpsvrdone();

// TODO cobol - move to own header
// COBOL Support
// This part ought to be in an internal header file. It is 
// special routines to support the COBOL api. It would
// be cleaner to not expose theese routines, but will try
// this first while prototyping.
// (The routines would then not need to be extern "C".)

// A function that returns a pointer to a copy of the
// arguments to the "in progress" service call, to allow
// the COBOL api TPSVCSTART routine to find the input
// to the service.  
extern int tpsvcinfo_cobol_support( const TPSVCINFO**,
                                    const char** buffer_type,
                                    const char** buffer_subtype);

// Variant of tpreturn() that returns instead of doing longjmp().
// Needed by COBOL api.
extern void tpreturn_cobol_support( int rval, long rcode, char* data, long len, long flags);

#ifdef __cplusplus
}
#endif

