/** 
 ** Copyright (c) 2020, The casual project
 **
 ** This software is licensed under the MIT license, https://opensource.org/licenses/MIT
 **/

#pragma once

/**
 * explicit symbols for the xatmi-interface, exactly the same semantics and implementation
 * as the real interface, just different symbols.
 * usefull if one wants to use casual at the same time as another xatmi implementation
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tpsvcinfo TPSVCINFO;

// tpalloc
extern char* casual_buffer_allocate( const char* type, const char* subtype, long size);
// tprealloc
extern char* casual_buffer_reallocate( const char* ptr, long size);
// tptypes
extern long casual_buffer_type( const char* buffer, char* type, char* subtype);
// tpfree
extern void casual_buffer_free( const char* ptr);

// tpcall
extern int casual_service_call( const char* svc, char* idata, long ilen, char** odata, long* olen, long flags);
// tpacall
extern int casual_service_asynchronous_send( const char* svc, char* idata, long ilen, long flags);
// tpgetrply
extern int casual_service_asynchronous_receive( int* idPtr, char ** odata, long* olen, long flags);
// tpcancel
extern int casual_service_asynchronous_cancel( int id); 

// tpreturn
extern void casual_service_return( int rval, long rcode, char* data, long len, long flags);
// tpadvertise
extern int casual_service_advertise( const char* svcname, void(*func)(TPSVCINFO *));
// tpunadvertise
extern int casual_service_unadvertise( const char* svcname);


#ifdef __cplusplus
}
#endif
