//!
//! sf_testproxy.cpp
//!
//! Created on: Mar 1, 2014
//!     Author: Lazan
//!


#include "sf_testproxy.h"

#include "sf/archive/archive.h"


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

         SomeService2::SomeService2() : SomeService2( 0) {}
         SomeService2::SomeService2( long flags) : m_service{ "casual_sf_test2", flags} {}


         SomeService2::Receive SomeService2::operator() ( const std::string& value)
         {
            m_service << CASUAL_MAKE_NVP( value);

            return Receive{ m_service()};
         }


         SomeService2::Receive::Receive( casual::sf::proxy::async::Receive&& receive)
            : m_receive{ std::move( receive)} {}



         std::vector< std::string> SomeService2::Receive::operator() ()
         {
            auto result = m_receive();

            std::vector< std::string> serviceReturnValue;

            result >> CASUAL_MAKE_NVP( serviceReturnValue);

            return serviceReturnValue;
         }


         // fler services...


      } // asynk

      namespace sync
      {

         SomeService2::SomeService2() : SomeService2( 0L) {}

         SomeService2::SomeService2( long flags) : m_service{ "casual_sf_test2", flags} {}


         std::vector< std::string> SomeService2::operator() ( const std::string& value)
         {
            m_service << CASUAL_MAKE_NVP( value);

            auto result = m_service();

            std::vector< std::string> serviceReturnValue;

            result >> CASUAL_MAKE_NVP( serviceReturnValue);

            return serviceReturnValue;
         }


         // fler services...

      } // sync



      std::vector< std::string> someService2( const std::string& value)
      {
         return sync::SomeService2()( value);
      }

   } // proxyName

} // test

} // casual


