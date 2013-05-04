//!
//! template_sf_server_implementation.h
//!
//! Created on: Jan 4, 2013
//!     Author: Lazan
//!

#ifndef TEMPLATE_SF_SERVER_IMPLEMENTATION_H_
#define TEMPLATE_SF_SERVER_IMPLEMENTATION_H_


#include "sf_testvo.h"

#include <vector>


namespace casual
{

   namespace test
   {

      class ServerImplementation
      {
      public:
         ServerImplementation( int argc, char** argv);
         ~ServerImplementation();


         bool casual_sf_test1( const std::vector< vo::TestVO>& inputValues, std::vector< vo::TestVO>& outputValues);

         void casual_sf_test2( bool someValue);

      private:

      };


   }
}




#endif /* TEMPLATE_SF_SERVER_IMPLEMENTATION_H_ */
