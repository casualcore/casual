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
extern void casual_test2( TPSVCINFO *transb);




int main( int argc, char** argv)
{

	struct casual_service_name_mapping mapping[] = {
			{&casual_test1, "casual_test1"},
			{&casual_test2, "casual_test2"}
	};

	/*
   // Initialize the server
   */
	int result = casual_initialize_server(
	      argc,
         argv,
         mapping,
         sizeof( mapping) / sizeof( struct casual_service_name_mapping));

	if( result != 0)
	   return result;

	/*
   // Let the user code get a chance being called via tpsvrinit
   */
	result = tpsvrinit( argc, argv);

	if( result != 0)
	   return result;

	/*
	// Start the server
	*/
	result = casual_start_server();

	/*
   // Let the user code get a chance being called via tpsvrdone
   */
	tpsvrdone();

	return result;
}







#ifdef __cplusplus
}
#endif



