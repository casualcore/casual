//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "sf_testvo.h"

// protected section


#include "common/server/start.h"
#include "sf/service/protocol.h"

namespace casual
{
   namespace test
   {
      namespace
      {
         namespace implementation
         {
            bool casual_sf_test1( std::vector< vo::TestVO> values, std::vector< vo::TestVO>& outputValues)
            {
               // protected section
               return true;
            }

            void casual_sf_test2( bool someValue)
            {
               // protected section
            }

         } // implementation




         namespace dispatch
         {
            common::service::invoke::Result casual_sf_test1( common::service::invoke::Parameter&& parameter)
            {
               auto protocol = sf::service::protocol::deduce( std::move( parameter));

               std::vector< vo::TestVO> values;
               protocol >> CASUAL_MAKE_NVP( values);

               std::vector< vo::TestVO> outputValues;

               auto result = sf::service::user( protocol, &implementation::casual_sf_test1, std::move( values), outputValues);

               protocol << CASUAL_MAKE_NVP( result);
               protocol << CASUAL_MAKE_NVP( outputValues);

               return protocol.finalize();
            }

            common::service::invoke::Result casual_sf_test2( common::service::invoke::Parameter&& parameter)
            {
               auto protocol = sf::service::protocol::deduce( std::move( parameter));

               bool someValue;
               protocol >> CASUAL_MAKE_NVP( someValue);

               sf::service::user( protocol, &implementation::casual_sf_test2, someValue);

               return protocol.finalize();
            }
         } // dispatch
      } // <unnamed>

   
      void main( int argc, char **argv)
      {
         // protected section

         common::server::start( {
            {
               "casual_sf_test1",
               &dispatch::casual_sf_test1
            },
            {
               "casual_sf_test2",
               &dispatch::casual_sf_test2
            }

         });

         // protected section
      }
   } // test
} // casual



int main( int argc, char **argv)
{
   return casual::common::server::main( argc, argv, &casual::test::main);
}



