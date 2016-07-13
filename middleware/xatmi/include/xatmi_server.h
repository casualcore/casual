//!
//! casual
//!

#ifndef XATMI_SERVER_H_
#define XATMI_SERVER_H_

#include <xa.h>
#include <xatmi.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


struct casual_service_name_mapping
{
	tpservice functionPointer;
	const char* name;

	/* type of service */
	uint64_t type;

	/* transaction policy */
	uint64_t transaction;

};


struct casual_xa_switch_mapping
{
   const char* key;
   struct xa_switch_t* xa_switch;
};


typedef void( *tpsvrdone_type)();
typedef int( *tpsvrinit_type)( int argc, char **argv);


struct casual_server_argument
{
   struct casual_service_name_mapping* services;

   tpsvrinit_type serviceInit;
   tpsvrdone_type serviceDone;

   int argc;
   char** argv;

   struct casual_xa_switch_mapping* xa_switches;
};



int casual_initialize_server( int argc, char** argv, struct casual_service_name_mapping* mapping, size_t size);

int casual_start_server( struct casual_server_argument* serverArgument);


#ifdef __cplusplus
}
#endif


#endif /* XATMI_SERVER_H_ */
