//!
//! template-server.cpp
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!
//!
//! Only to get a feel for the abstractions needed.
//! That is, to go "from the outside -> in" instead of the opposite
//!

#include "template_server.h"


void casual_test1( TPSVCINFO *transb)
{
	
	char* buffer = tpalloc( "STRING", 0, 500);


	buffer = tprealloc( buffer, 3000);
}

void casual_test2( TPSVCINFO *transb)
{
	//
	// TODO:
	// This method right now is just to remove linkage error
	//
	char* buffer = tpalloc( "STRING", 0, 500);


	buffer = tprealloc( buffer, 3000);
}






