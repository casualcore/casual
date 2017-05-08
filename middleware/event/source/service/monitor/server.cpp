//!
//! casual 
//!


#include "common/error.h"
#include "common/service/invoke.h"


namespace casual
{
   namespace event
   {
      namespace service
      {
         namespace monitor
         {
            common::service::invoke::Result metrics( common::service::invoke::Parameter&& argument)
            {


               return std::move( argument.payload);
            }



            void main( int argc, char **argv)
            {

            }

         } // monitor
      } // service
   } // event
} // casual


int main( int argc, char **argv)
{
   try
   {
      casual::event::service::monitor::main( argc, argv);
      return 0;
   }
   catch( ...)
   {
      return casual::common::error::handler();
   }
}

