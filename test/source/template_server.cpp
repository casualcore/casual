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


#include "casual_logger.h"

#include <iostream>

#include <unistd.h>




#include "xatmi.h"

void casual_test1( TPSVCINFO *serviceContext)
{


   char* buffer = tpalloc( "STRING", 0, 500);
   buffer = tprealloc( buffer, 3000);

   {
      std::cout << "transb->name: " << serviceContext->name << std::endl;
      std::cout << "transb->cd: " << serviceContext->cd << std::endl;
      std::cout << "transb->data: " << serviceContext->data << std::endl;



      std::string test = "bla bla bla";
      std::copy( test.begin(), test.end(), buffer);
      buffer[ test.size()] = '\0';
   }


	tpreturn( TPSUCCESS, 0, buffer, 3000, 0);


}

void casual_test2( TPSVCINFO *serviceContext)
{

   //sleep( 1);

	tpreturn( TPSUCCESS, 0, serviceContext->data, serviceContext->len, 0);
}


int tpsvrinit(int argc, char **argv)
{
   casual::logger::debug << "USER tpsvrinit called";
   return 0;
}






