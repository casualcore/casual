//!
//! xatmi_server.h
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!

#ifndef XATMI_SERVER_H_
#define XATMI_SERVER_H_

#include "xatmi.h"
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif


struct casual_service_name_mapping
{
	tpservice m_functionPointer;
	const char* m_name;
};


typedef void( *tpsvrdone_type)();
typedef int( *tpsvrinit_type)( int argc, char **argv);


struct casual_server_argument
{
   casual_service_name_mapping* m_serverStart;
   casual_service_name_mapping* m_serverEnd;

   tpsvrinit_type m_serviceInit;
   tpsvrdone_type m_serviceDone;

   int m_argc;
   char** m_argv;


};



int casual_initialize_server( int argc, char** argv, struct casual_service_name_mapping* mapping, size_t size);

int casual_start_server( casual_server_argument* serverArgument);


#ifdef __cplusplus
}
#endif


#endif /* XATMI_SERVER_H_ */
