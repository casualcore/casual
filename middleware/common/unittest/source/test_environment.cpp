//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>
#include "common/unittest.h"

#include "common/environment.h"
#include "common/environment/string.h"


namespace casual
{
   namespace common
   {
      TEST( common_environment, string__no_variable__expect_same)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( environment::string( "test/a/b/c") == "test/a/b/c");
      }


      TEST( common_environment, environment_string__variable_at_beginning__expect_altered)
      {
         common::unittest::Trace trace;

         ASSERT_TRUE( environment::variable::exists( "HOME"));

         auto home = environment::variable::get( "HOME");

         auto result =  environment::string( "${HOME}/a/b/c");

         EXPECT_TRUE( result == home + "/a/b/c") << "result: " << result;
      }

      TEST( common_environment, environment_string__variable_in_middle__expect_altered)
      {
         common::unittest::Trace trace;

         ASSERT_TRUE( environment::variable::exists( "HOME"));

         auto home = environment::variable::get( "HOME");

         auto result =  environment::string( "a/b/c/${HOME}/a/b/c");

         EXPECT_TRUE( result == "a/b/c/" + home + "/a/b/c") << "result: " << result;
      }

      TEST( common_environment, string_variable__expect_altered)
      {
         common::unittest::Trace trace;

         ASSERT_TRUE( environment::variable::exists( "HOME"));

         auto home = environment::variable::get( "HOME");

         auto result =  environment::string( "${HOME}/a/b/c");

         EXPECT_TRUE( result == home + "/a/b/c") << "result: " << result;
      }

      TEST( common_environment, string_variable__not_correct_format__expect_no_alteration)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( environment::string( "${HOME/a/b/c") == "${HOME/a/b/c");
      }

      TEST( common_environment, process___expect_serialized)
      {
         common::unittest::Trace trace;

         auto process = common::process::handle();

         environment::variable::process::set( "TEST_PROCESS_VARIABLE", process);

         EXPECT_TRUE( process == environment::variable::process::get( "TEST_PROCESS_VARIABLE"));

      }

      TEST( common_environment, environment_CASUAL_HOME__variable_in_middle__expect_altered)
      {
         common::unittest::Trace trace;

         environment::variable::set( "CASUAL_HOME", "test");

         auto result =  environment::string( "main.cpp -I${CASUAL_HOME}/include");

         EXPECT_TRUE( result == "main.cpp -Itest/include") << "result: " << result;
      }

      TEST( common_environment, environment_CASUAL_HOME__variable_before_dash_I__expect_altered)
      {
         common::unittest::Trace trace;

         environment::variable::set( "CASUAL_HOME", "test");

         auto result =  environment::string( "-I${CASUAL_HOME}/include");

         EXPECT_TRUE( result == "-Itest/include") << "result: " << result;
      }

      TEST( common_environment, local_repository)
      {
         common::unittest::Trace trace;

         std::vector< environment::Variable> local{ { "FOO=foo"}, {"BAR=bar"}};

         auto result = environment::string( "${FOO}${BAR}", local);

         EXPECT_TRUE( result == "foobar") << "result: " << result;
      }

      TEST( common_environment, local_repository_complex)
      {
         common::unittest::Trace trace;

         std::vector< environment::Variable> local{ { "FOO=foo"}, {"BAR=bar"}};

         auto result = environment::string( "a${FOO}b${BAR}c", local);

         EXPECT_TRUE( result == "afoobbarc") << "result: " << result;
      }

      TEST( common_environment, local_repository__fallback_environment)
      {
         common::unittest::Trace trace;

         environment::variable::set( "POOP", "poop");

         std::vector< environment::Variable> local{ { "FOO=foo"}, {"BAR=bar"}};

      
         auto result = environment::string( "${FOO}${POOP}${BAR}", local);

         EXPECT_TRUE( result == "foopoopbar") << "result: " << result;
      }  

   } // common
} // casual




