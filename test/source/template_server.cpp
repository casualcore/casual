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


#include "common/logger.h"
#include "common/process.h"

#include "common/arguments.h"
#include "common/string.h"
#include "common/environment.h"


#include <unistd.h>




#include "xatmi.h"

extern "C"
{


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
   auto args = casual::common::string::split( serviceContext->data);

   casual::common::Arguments parser;

   std::size_t millesconds = 0;

   parser.add(
         casual::common::argument::directive( { "-ms", "--ms-sleep"}, "sleep time", millesconds)
   );

   parser.parse( casual::common::environment::file::executable(), args);

   casual::common::logger::debug << "casual_test2 called - sleep for a while...";

   casual::common::process::sleep(  std::chrono::milliseconds( millesconds));

	tpreturn( TPSUCCESS, 0, serviceContext->data, serviceContext->len, 0);
}


void casual_test3( TPSVCINFO *serviceContext)
{

   casual::common::logger::debug << "casual_test3 called";

   tpreturn( TPSUCCESS, 0, serviceContext->data, serviceContext->len, 0);
}




int tpsvrinit(int argc, char **argv)
{
   casual::common::logger::debug << "USER tpsvrinit called";

   return 0;
}

}






