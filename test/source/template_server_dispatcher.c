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




#include <xatmi.h>
#include <xatmi_server.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void casual_test1( TPSVCINFO *transb);
extern void casual_test2( TPSVCINFO *transb);




int main( int argc, char** argv)
{

	struct casual_service_name_mapping mapping[] = {
			{&casual_test1, "casual_test1"},
			{&casual_test2, "casual_test2"}
	};

	struct casual_server_argument serverArguments = {
	      &mapping[ 0],
	      &mapping[ sizeof( mapping) / sizeof( struct casual_service_name_mapping) - 1] + 1,
	      &tpsvrinit,
	      &tpsvrdone,
	      argc,
	      argv
	};


	/*
	// Start the server
	*/
	return casual_start_server( &serverArguments);

}







#ifdef __cplusplus
}
#endif



