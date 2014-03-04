//!
//! sf_testproxy.cpp
//!
//! Created on: Mar 1, 2014
//!     Author: Lazan
//!


#include "sf_testproxy.h"

#include "sf/archive.h"


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

         Casual_sf_test1::Casual_sf_test1() : Casual_sf_test1( 0) {}

         Casual_sf_test1::Casual_sf_test1( long flags) : casual::sf::proxy::Async( "casual_sf_test1", flags)
         {

         }

         void Casual_sf_test1::send( const std::string& value)
         {
            //## protected section begin [666.100]
            //## protected section end   [666.100]

            auto&& helper = interface();

            helper << CASUAL_MAKE_NVP( value);

            //## protected section begin [666.120]
            //## protected section end   [666.120]

            helper.send();
         }


         std::vector< std::string> Casual_sf_test1::receive()
         {
            //## protected section begin [666.150]
            //## protected section end   [666.120]

            auto&& helper = interface();

            std::vector< std::string> serviceReturnValue;

            helper >> CASUAL_MAKE_NVP( serviceReturnValue);

            //## protected section begin [666.170]
            //## protected section end   [666.170]

            helper.finalize();

            return serviceReturnValue;
         }


         // fler services...


      } // asynk

      namespace sync
      {

         Casual_sf_test1::Casual_sf_test1() : Casual_sf_test1( 0) {}

         Casual_sf_test1::Casual_sf_test1( long flags) : casual::sf::proxy::Sync( "casual_sf_test1", flags)
         {

         }


         std::vector< std::string> Casual_sf_test1::call( const std::string& value)
         {
            //## protected section begin [666.200]
            //## protected section end   [666.200]

            auto&& helper = interface();

            helper << CASUAL_MAKE_NVP( value);

            helper.call();

            std::vector< std::string> serviceReturnValue;

            helper >> CASUAL_MAKE_NVP( serviceReturnValue);

            //## protected section begin [666.280]
            //## protected section end   [666.280]

            helper.finalize();

            return serviceReturnValue;
         }


         // fler services...

      } // asynk



      std::vector< std::string> casual_sf_test1( const std::string& value)
      {
         return sync::Casual_sf_test1().call( value);
      }

   } // proxyName

} // test

} // casual

