//!
//! casual
//!

#include "gateway/outbound/ipc/connector.h"


#include "common/arguments.h"

namespace casual
{

   namespace gateway
   {
      namespace outbound
      {

         int main( int argc, char **argv)
         {
            try
            {
               ipc::Settings settings;
               {
                  casual::common::Arguments parser{{

                  }};
                  parser.parse( argc, argv);
               }

               ipc::Connector connector{ std::move( settings)};
               connector.start();

            }
            catch( ...)
            {
               return casual::common::error::handler();
            }
            return 0;
         }

      } // outbound
   } // gateway


} // casual


int main( int argc, char **argv)
{
   return casual::gateway::outbound::main( argc, argv);
}
