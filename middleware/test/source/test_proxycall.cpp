//!
//! test_proxycall.cpp
//!
//! Created on: Mar 8, 2014
//!     Author: Lazan
//!

#include "sf_testproxy.h"

#include <vector>
#include <string>
#include <algorithm>

namespace casual
{
   namespace sf
   {


   } // sf

} // casual



int main( int argc, char** argv)
{

   // use case 1
   {
      casual::test::proxyName::async::SomeService2 someService2;

      auto receive = someService2( "someValue");

      // ... do some other stuff

      auto result = receive();
   }

   // use case 2
   {
      typedef casual::test::proxyName::async::SomeService2 SomeService2;

      auto receive = SomeService2()( "someValue");

      // ... do some other stuff

      auto result = receive();

   }

   // use case 3
   {
      casual::test::proxyName::async::SomeService2 someService2;

      auto receive1 = someService2( "someValue");
      auto receive2 = someService2( "someValue");

      // ... do some other stuff

      auto result1 = receive1();
      auto result2 = receive2();
   }

   // use case 4
   {
      typedef casual::test::proxyName::async::SomeService2 SomeService2;
      //SomeService2 someService2;

      std::vector< std::string> values = { "value1", "value2", "value3", "value4"};

      std::vector< SomeService2::Receive> receivers;

      std::transform(
            values.begin(),
            values.end(),
            std::back_inserter( receivers),
            SomeService2{});

      // do some other stuff


      for( auto&& receive : receivers)
      {
         auto result = receive();
         // do stuff with result
      }

   }




   return 0;
}

