//!
//! casual
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "common/environment.h"
#include "common/exception/system.h"


namespace casual
{
   namespace common
   {
      TEST( casual_common_environment, string__no_variable__expect_same)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( environment::string( "test/a/b/c") == "test/a/b/c");
      }

      TEST( casual_common_environment, environment_string__variable_in_middle__expect_altered)
      {
         common::unittest::Trace trace;

         ASSERT_TRUE( environment::variable::exists( "HOME"));

         auto home = environment::variable::get( "HOME");

         auto result =  environment::string( "a/b/c/${HOME}/a/b/c");

         EXPECT_TRUE( result == "a/b/c/" + home + "/a/b/c") << "result: " << result;
      }

      TEST( casual_common_environment, string_variable__expect_altered)
      {
         common::unittest::Trace trace;

         ASSERT_TRUE( environment::variable::exists( "HOME"));

         auto home = environment::variable::get( "HOME");

         auto result =  environment::string( "${HOME}/a/b/c");

         EXPECT_TRUE( result == home + "/a/b/c") << "result: " << result;
      }

      TEST( casual_common_environment, string_variable__not_correct_format__expect_throw)
      {
         common::unittest::Trace trace;

         EXPECT_THROW(
         {
            environment::string( "${HOME/a/b/c");
         }, exception::system::invalid::Argument);
      }

      TEST( casual_common_environment, process___expect_serialized)
      {
         common::unittest::Trace trace;

         auto process = common::process::handle();

         environment::variable::process::set( "TEST_PROCESS_VARIABLE", process);

         EXPECT_TRUE( process == environment::variable::process::get( "TEST_PROCESS_VARIABLE"));

      }

      TEST( casual_common_environment, environment_CASUAL_HOME__variable_in_middle__expect_altered)
      {
         common::unittest::Trace trace;

         environment::variable::set( "CASUAL_HOME", "test");

         auto result =  environment::string( "main.cpp -I${CASUAL_HOME}/include");

         EXPECT_TRUE( result == "main.cpp -Itest/include") << "result: " << result;
      }

      TEST( casual_common_environment, environment_CASUAL_HOME__variable_before_dash_I__expect_altered)
      {
         common::unittest::Trace trace;

         environment::variable::set( "CASUAL_HOME", "test");

         auto result =  environment::string( "-I${CASUAL_HOME}/include");

         EXPECT_TRUE( result == "-Itest/include") << "result: " << result;
      }
   } // common
} // casual




