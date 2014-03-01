//!
//! sf_testproxy.cpp
//!
//! Created on: Mar 1, 2014
//!     Author: Lazan
//!


#include "sf_testproxy.h"


//## includes protected section begin [.10]
#include <string>
#include <vector>
//## includes protected section end   [.10]


namespace casual
{

namespace test
{
   //## declarations protected section begin [.20]
   //## declarations protected section end   [.20]

   namespace proxyName
   {
      //## declarations protected section begin [.30]
      //## declarations protected section end   [.30]

      namespace async
      {
         //## declarations protected section begin [.100]
         //## declarations protected section end   [.100]

         void Casual_sf_test1::send( const std::string)
         {
            //## protected section begin [666.100]
            //## protected section end   [666.100]

            // some more code...

            //## protected section begin [666.120]
            //## protected section end   [666.120]

         }


         std::vector< std::string> Casual_sf_test1::receive()
         {
            //## protected section begin [666.150]
            //## protected section end   [666.120]

            // some more code...

            //## protected section begin [666.170]
            //## protected section end   [666.170]
            return {};
         }


         // fler services...


      } // asynk

      namespace sync
      {
         std::vector< std::string> Casual_sf_test1::call( const std::string& value)
         {
            //## protected section begin [666.200]
            //## protected section end   [666.200]

            // some more code


            //## protected section begin [666.280]
            //## protected section end   [666.280]

            return {};
         }


         // fler services...

      } // asynk



      std::vector< std::string> casual_sf_test1( const std::string& value)
      {
         return sync::Casual_sf_test1().call( value);
      }



   };



} // test

} // casual

