

//## includes protected section begin [.10]

#include "sf_testserverimplementation.h"

//## includes protected section end   [.10]

namespace casual
{
namespace test
{


//## declarations protected section begin [.20]
//## declarations protected section end   [.20]

TestServerImplementation::TestServerImplementation( int argc, char **argv)
{
   //## constructor protected section begin [ctor.20]
   //## constructor protected section end   [ctor.20]
 }



TestServerImplementation::~TestServerImplementation()
{
   //## destructor protected section begin [dtor.20]
   //## destructor protected section end   [dtor.20]
}


//
// Services definitions
//


bool TestServerImplementation::casual_sf_test1( const std::vector< vo::TestVO>& values, std::vector< vo::TestVO>& outputValues)
{
   //## service implementation protected section begin [501]
   return false;
   //## service implementation protected section end   [501]
}

void TestServerImplementation::casual_sf_test2( bool someValue)
{
   //## service implementation protected section begin [502]
   //## service implementation protected section end   [502]
}

//## declarations protected section begin [.40]
//## declarations protected section end   [.40]

} // test
} // casual

//## declarations protected section begin [.50]
//## declarations protected section end   [.50]
