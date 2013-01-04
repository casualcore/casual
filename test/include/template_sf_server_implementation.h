//!
//! template_sf_server_implementation.h
//!
//! Created on: Jan 4, 2013
//!     Author: Lazan
//!

#ifndef TEMPLATE_SF_SERVER_IMPLEMENTATION_H_
#define TEMPLATE_SF_SERVER_IMPLEMENTATION_H_


#include "template_vo.h"

#include <vector>



namespace test
{

   class ServerImplementation
   {
   public:
      ServerImplementation( int argc, char** argv);
      ~ServerImplementation();


      bool casual_sf_test1( const std::vector< vo::Value>& inputValues, std::vector< vo::Value>& outputValues);

   private:

   };


}




#endif /* TEMPLATE_SF_SERVER_IMPLEMENTATION_H_ */
