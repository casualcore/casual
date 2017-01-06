//!
//! casual 
//!

#include "configuration/domain.h"

#include "configuration/example/domain.h"

#include "sf/namevaluepair.h"
#include "sf/archive/maker.h"


#include "common/arguments.h"

namespace casual
{
   namespace configuration
   {
      namespace example
      {

         void create(  const std::string& file)
         {
            write( example::domain(), file);
         }

         void create_default( const std::string& file)
         {
            write( domain::Manager{}, file);
         }


         int main( int argc, char **argv)
         {
            try
            {

               common::Arguments argument{
                  {
                     common::argument::directive( { "-o", "--output"}, "output file - format will be deduced from extension", &create),
                     common::argument::directive( { "-d", "--default"}, "output default configuration", &create_default)
                  }
               };

               argument.parse( argc, argv);

               return 0;
            }
            catch( ...)
            {
               return common::error::handler();
            }
         }

      } // example

   } // configuration

} // casual

int main( int argc, char **argv)
{
   return casual::configuration::example::main( argc, argv);
}
