//!
//! template-server.h
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!
//! Only to get a feel for the abstractions needed.
//! That is, to go "from the outside -> in" istead of the opposite
//!

#ifndef TEMPLATE_SERVER_H_
#define TEMPLATE_SERVER_H_


#include <xatmi.h>



extern "C"
{
   int tpsvrinit(int argc, char **argv);

	void casual_test1( TPSVCINFO *transb);
	void casual_test2( TPSVCINFO *transb);

}





#endif /* TEMPLATE_SERVER_H_ */
