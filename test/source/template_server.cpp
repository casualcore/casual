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

#include <iostream>


#include "xatmi.h"

void casual_test1( TPSVCINFO *transb)
{

   std::cout << "transb->name: " << transb->name << std::endl;
   std::cout << "transb->cd: " << transb->cd << std::endl;
   std::cout << "transb->data: " << transb->data << std::endl;
	
	char* buffer = tpalloc( "STRING", 0, 500);
	buffer = tprealloc( buffer, 3000);

	std::string test = "bla bla bla";
	std::copy( test.begin(), test.end(), buffer);
	buffer[ test.size()] = '\0';


	tpreturn( TPSUCCESS, 0, buffer, 3000, 0);


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






