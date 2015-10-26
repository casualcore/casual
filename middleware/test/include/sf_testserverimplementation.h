#ifndef CASUALTESTTESTSERVERIMPLEMENTATION_H
#define CASUALTESTTESTSERVERIMPLEMENTATION_H


//## includes protected section begin [.10]

#include "sf_testvo.h"

#include <string>
#include <vector>

//## includes protected section end   [.10]

namespace casual
{
namespace test
{


//## declarations protected section begin [.20]
//## declarations protected section end   [.20]


class TestServerImplementation
{

public:
    
   //
   // Constructor and destructor. 
   // use these to initialize state if needed
   //
   TestServerImplementation( int argc, char **argv);
    
   ~TestServerImplementation();
    

   //## declarations protected section begin [.200]
   //## declarations protected section end   [.200]
    
    
   //
   // Services
   //
    
   
   //!
   //! Does some stuff 
   //! 
   //! @return true if some condition is met
   //! @param values holds some values
   //!
   bool casual_sf_test1( const std::vector< vo::TestVO>& values, std::vector< vo::TestVO>& outputValues);

   //!
   //! 
   //! Secret stuff...
   //!
   void casual_sf_test2( bool someValue);

   //## declarations protected section begin [.300]
   //## declarations protected section end   [.300]

};


//## declarations protected section begin [.40]
//## declarations protected section end   [.40]

} // test
} // casual

//## declarations protected section begin [.50]
//## declarations protected section end   [.50]
#endif 
