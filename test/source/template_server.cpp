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


#include "common/logger.h"


#include <unistd.h>




#include "xatmi.h"

void casual_test1( TPSVCINFO *serviceContext)
{


   char* buffer = tpalloc( "STRING", 0, 500);
   buffer = tprealloc( buffer, 3000);

   {
      casual::common::logger::debug << "transb->name: " << serviceContext->name;
      casual::common::logger::debug << "transb->cd: " << serviceContext->cd;
      casual::common::logger::debug << "transb->data: " << serviceContext->data;



      std::string test = "bla bla bla";
      std::copy( test.begin(), test.end(), buffer);
      buffer[ test.size()] = '\0';
   }


	tpreturn( TPSUCCESS, 0, buffer, 3000, 0);


}

void casual_test2( TPSVCINFO *serviceContext)
{

   casual::common::logger::debug << "casual_test2 called";

	tpreturn( TPSUCCESS, 0, serviceContext->data, serviceContext->len, 0);
}


void casual_test3( TPSVCINFO *serviceContext)
{

   casual::common::logger::debug << "casual_test2 called";

   tpreturn( TPSUCCESS, 0, serviceContext->data, serviceContext->len, 0);
}




int tpsvrinit(int argc, char **argv)
{
   casual::common::logger::debug << "USER tpsvrinit called";

   return 0;
}








