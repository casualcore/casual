//!
//! template_sf_server_implementation.cpp
//!
//! Created on: Jan 4, 2013
//!     Author: Lazan
//!


#include "template_sf_server_implementation.h"

namespace casual
{

namespace test
{

   ServerImplementation::ServerImplementation( int argc, char** argv)
   {
   }

   ServerImplementation::~ServerImplementation()
   {

   }

   bool ServerImplementation::casual_sf_test1( std::vector< vo::TestVO>& outputValues)
   {

      outputValues.resize( 3);


      return true;

   }

}

}



