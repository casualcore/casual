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


#include "common/log.h"
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
      casual::common::log::debug << "transb->name: " << serviceContext->name << std::endl;
      casual::common::log::debug << "transb->cd: " << serviceContext->cd << std::endl;
      casual::common::log::debug << "transb->data: " << serviceContext->data << std::endl;



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

   casual::common::log::debug << "casual_test2 called - sleep for a while..." << std::endl;

   casual::common::process::sleep(  std::chrono::milliseconds( millesconds));

	tpreturn( TPSUCCESS, 0, serviceContext->data, serviceContext->len, 0);
}


void casual_test3( TPSVCINFO *serviceContext)
{

   casual::common::log::debug << "casual_test3 called" << std::endl;

   tpreturn( TPSUCCESS, 0, serviceContext->data, serviceContext->len, 0);
}




int tpsvrinit(int argc, char **argv)
{
   casual::common::log::debug << "USER tpsvrinit called" << std::endl;

   return 0;
}

}






