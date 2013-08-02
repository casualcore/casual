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


extern struct xa_switch_t db2xa_switch_static_std;


int main( int argc, char** argv)
{

	struct casual_service_name_mapping service_mapping[] = {
			{&casual_test1, "casual_test1"},
			{&casual_test2, "casual_test2"},
			{ 0, 0} /* null ending */
	};

#ifdef __cplusplus
	static_assert( sizeof( "casual_test1") <= XATMI_SERVICE_NAME_LENGTH, "service name to long");
#endif


	struct casual_xa_switch_mapping xa_mapping[] = {
	        { "db2", &db2xa_switch_static_std},
	        { 0, 0} /* null ending */
	   };


	struct casual_server_argument serverArguments = {
	      service_mapping,
	      &tpsvrinit,
	      &tpsvrdone,
	      argc,
	      argv,
	      xa_mapping
	};


	/*
	// Start the server
	*/
	return casual_start_server( &serverArguments);

}







#ifdef __cplusplus
}
#endif



