//!
//! xatmi_server.h
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!

#ifndef XATMI_SERVER_H_
#define XATMI_SERVER_H_

#include <xatmi.h>
#include <xa.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif


struct casual_service_name_mapping
{
	tpservice functionPointer;
	const char* name;
};


struct casual_xa_switch_mapping
{
   const char* key;
   xa_switch_t* xaSwitch;
};


typedef void( *tpsvrdone_type)();
typedef int( *tpsvrinit_type)( int argc, char **argv);


struct casual_server_argument
{
   casual_service_name_mapping* services;

   tpsvrinit_type serviceInit;
   tpsvrdone_type serviceDone;

   int argc;
   char** argv;

   casual_xa_switch_mapping* xaSwitches;
};



int casual_initialize_server( int argc, char** argv, struct casual_service_name_mapping* mapping, size_t size);

int casual_start_server( casual_server_argument* serverArgument);


#ifdef __cplusplus
}
#endif


#endif /* XATMI_SERVER_H_ */
