
#include <gtest/gtest.h>


#include "common/environment.h"


namespace casual
{
   namespace common
   {
      TEST( casual_common_environment, string__no_variable__expect_same)
      {

         EXPECT_TRUE( environment::string( "test/a/b/c") == "test/a/b/c");
      }

      TEST( casual_common_environment, string_variable__expect_altered)
      {
         ASSERT_TRUE( environment::variable::exists( "HOME"));

         auto home = environment::variable::get( "HOME");

         auto result =  environment::string( "$(HOME)/a/b/c");

         EXPECT_TRUE( result == home + "/a/b/c") << "result: " << result;
      }

      TEST( casual_common_environment, string_variable__not_correct_format__expect_same)
      {

         auto result =  environment::string( "$(HOME/a/b/c");

         EXPECT_TRUE( result == "$(HOME/a/b/c") << "result: " << result;
      }
   } // common
} // casual




