//!
//! caual_server_dispatcher.c
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!
//!
//! Template for the generated file that will define main()
//! and the function to servicename mapping + maybe some other init stuff.
//!


#ifdef __cplusplus
extern "C" {
#endif

#include <xatmi.h>
#include <xatmi_server.h>


extern void casual_test1( TPSVCINFO *transb);




int main( int argc, char** argv)
{

	struct casual_service_name_mapping mapping[] = { &casual_test1, "casual_test1" };

	return casual_startServer(
			argc,
			argv,
			mapping,
			sizeof( mapping) / sizeof( struct casual_service_name_mapping));

}







#ifdef __cplusplus
}
#endif



