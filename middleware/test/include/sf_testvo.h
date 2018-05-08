//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once
 

// 
// std
//
#include <memory>
    
//
// Forwards
//
namespace casual 
{ 
   namespace serviceframework 
   { 
      namespace archive 
      {
         class Reader;
         class Writer;
      }
    }
}
    
//## includes protected section begin [200.10]
//## includes protected section end   [200.10]

//## additional declarations protected section begin [200.20]
//## additional declarations protected section end   [200.20]

namespace casual
{
namespace test
{
namespace vo
{


class TestVO
{
public:

   //## additional public declarations protected section begin [200.100]
   //## additional public declarations protected section end   [200.100]


   TestVO();
   ~TestVO();
   TestVO( const TestVO&);
   TestVO& operator = ( const TestVO&);
   TestVO( TestVO&&);
   TestVO& operator = (TestVO&&);


   //## additional public declarations protected section begin [200.110]
   //## additional public declarations protected section end   [200.110]
   //
   // Getters and setters
   //
   
   
   long getSomeLong() const;
   void setSomeLong( long value);


   std::string getSomeString() const;
   void setSomeString( std::string value);



  
   void serialize( casual::serviceframework::archive::Reader& archive);
  
   void serialize( casual::serviceframework::archive::Writer& archive) const;

private:


   //## additional private declarations protected section begin [200.200]
   //## additional private declarations protected section end   [200.200]

   struct Implementation;
   std::unique_ptr< Implementation> pimpl;


};


} // vo
} // test
} // casual


