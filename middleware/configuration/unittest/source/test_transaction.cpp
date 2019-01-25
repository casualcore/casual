//!
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "configuration/transaction.h"


namespace casual
{
   namespace configuration
   {
      TEST( configuration_transaction, compound_assign)
      {
         common::unittest::Trace trace;

         auto lhs = []()
         {
            transaction::Manager result;

            result.log = "foo";
            result.manager_default.resource.instances = 2;
            result.manager_default.resource.key = std::string( "rm-key-1");
            result.resources = {
               [](){
                  transaction::Resource result;

                  result.name = "rm-name-1";
                  result.instances = 3;
                  result.openinfo = std::string{ "openinfo-1"};
                  result.closeinfo = std::string{ "closeinfo-1"};

                  return result;
               }()
            };

            return result;
         }();

         auto rhs = []()
         {
            transaction::Manager result;

            result.log = "bar";
            result.manager_default.resource.instances = 2;
            result.manager_default.resource.key = std::string( "rm-key-2");
            result.resources = {
               [](){
                  transaction::Resource result;

                  result.name = "rm-name-2";
                  result.instances = 2;
                  result.openinfo = std::string{ "openinfo-2"};
                  result.closeinfo = std::string{ "closeinfo-2"};

                  return result;
               }()
            };

            return result;
         }();


         lhs += rhs;

         EXPECT_TRUE( lhs.log == "bar");
         ASSERT_TRUE( lhs.resources.size() == 2);
         EXPECT_TRUE( lhs.resources.at( 0).name == "rm-name-1");
         EXPECT_TRUE( lhs.resources.at( 1).name == "rm-name-2");

      }


   } // configuration
} // casual
