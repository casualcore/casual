//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <serviceframework/archive/archive.h>


//## includes protected section begin [200.20]

#include "sf_testvo.h"

//## includes protected section end   [200.20]

namespace casual
{
namespace test
{
namespace vo
{



struct TestVO::Implementation
{
    Implementation() // NOLINT
   //## initialization list protected section begin [200.40]
         
   {
      //## ctor protected section begin [200.impl.ctor.10]
      //## ctor protected section end   [200.impl.ctor.10]
   }

 
   template< typename A>
   void serialize( A& archive)
   {
      //## additional serialization protected section begin [200.impl.serial.10]
      //## additional serialization protected section end   [200.impl.serial.10]
      archive & CASUAL_MAKE_NVP( someLong);
      archive & CASUAL_MAKE_NVP( someString);
      //## additional serialization protected section begin [200.impl.serial.20]
      //## additional serialization protected section end   [200.impl.serial.20]
   }

   //## additional attributes protected section begin [200.impl.attr.10]
   //## additional attributes protected section end   [200.impl.attr.10]
   long someLong{ 0};
   std::string someString;
   //## additional attributes protected section begin [200.impl.attr.20]
   //## additional attributes protected section end   [200.impl.attr.20]

};




TestVO::TestVO()
   : pimpl( new Implementation())  
{
   //## base class protected section begin [200.ctor.10]
   //## base class protected section end   [200.ctor.10]  
}

TestVO::~TestVO() = default;

TestVO::TestVO( TestVO&&  rhs) = default;

TestVO& TestVO::operator = (TestVO&&) = default;


TestVO::TestVO( const TestVO& rhs)
   : pimpl( new Implementation( *rhs.pimpl))
{

}

TestVO& TestVO::operator = ( const TestVO& rhs)
{
    *pimpl = *rhs.pimpl;
    return *this;
}

long TestVO::getSomeLong() const
{
   return pimpl->someLong;
}
std::string TestVO::getSomeString() const
{
   return pimpl->someString;
}


void TestVO::setSomeLong( long value)
{
   pimpl->someLong = value;
}
void TestVO::setSomeString( std::string value)
{
   pimpl->someString = value;
}






void TestVO::serialize( casual::serviceframework::archive::Reader& archive)
{
    pimpl->serialize( archive);
}
  
void TestVO::serialize( casual::serviceframework::archive::Writer& archive) const
{
    pimpl->serialize( archive);
}




} // vo
} // test
} // casual

