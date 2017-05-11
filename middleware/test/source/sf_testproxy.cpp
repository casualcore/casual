//!
//! casual
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

      namespace local
      {
         namespace
         {
            namespace someService2
            {
               template< typename C>
               void input( C& caller, const std::string& value)
               {
                  caller << CASUAL_MAKE_NVP( value);
               }

               template< typename R>
               auto output( R& reply)
               {
                  std::vector< std::string> result;
                  reply >> CASUAL_MAKE_NVP( result);
                  return result;
               }
            } // someService2
         } // <unnamed>
      } // local

      namespace send
      {

         SomeService2::SomeService2() = default;
         SomeService2::~SomeService2() = default;

         SomeService2::Receive SomeService2::operator() ( const std::string& value, Flags flags)
         {
            sf::service::protocol::binary::Send send;

            local::someService2::input( send, value);

            return Receive{ send( "casual_sf_test2")};
         }

         SomeService2::Receive SomeService2::operator() ( const std::string& value)
         {
            return operator() ( value, Flag::no_flags);
         }


         SomeService2::Receive::Receive( receive_type&& receive)
            : m_receive{ std::move( receive)} {}



         std::vector< std::string> SomeService2::Receive::operator() ()
         {
            auto result = m_receive();

            return local::someService2::output( result);
         }


         // fler services...


      } // send

      namespace call
      {
         std::vector< std::string> someService2( const std::string& value, Flags flag)
         {
            sf::service::protocol::binary::Call call;
            local::someService2::input( call, value);

            auto reply = call( "casual_sf_test2", flag);

            return local::someService2::output( reply);
         }

         std::vector< std::string> someService2( const std::string& value)
         {
            return someService2( value, Flag::no_flags);
         }


         SomeService2::SomeService2() = default;
         SomeService2::~SomeService2() = default;

         std::vector< std::string> SomeService2::operator() ( const std::string& value)
         {
            return someService2( value);
         }

         std::vector< std::string> SomeService2::operator() ( const std::string& value, Flags flag)
         {
            return someService2( value, flag);
         }


         // fler services...

      } // call





   } // proxyName

} // test

} // casual


