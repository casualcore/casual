//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>
#include "common/unittest.h"

#include "common/environment.h"
#include "common/environment/expand.h"


namespace casual
{
   namespace common
   {
      TEST( common_environment_variable_type, boolean_default)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( ! environment::variable::get( "SOME_VARIABLE", false));
         EXPECT_TRUE( environment::variable::get( "SOME_VARIABLE", true));
      }

      TEST( common_environment_variable_type, boolean)
      {
         common::unittest::Trace trace;

         environment::variable::set( "SOME_VARIABLE_5b54338bc2704", true);
         EXPECT_TRUE( environment::variable::get< bool>( "SOME_VARIABLE_5b54338bc2704"));

         environment::variable::set( "SOME_VARIABLE_5b54338bc2704", false);
         EXPECT_TRUE( ! environment::variable::get< bool>( "SOME_VARIABLE_5b54338bc2704"));
      }

      TEST( common_environment, string__no_variable__expect_same)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( environment::expand( std::filesystem::path{ "test/a/b/c"}) == "test/a/b/c");
      }


      TEST( common_environment, environment_string__variable_at_beginning__expect_altered)
      {
         common::unittest::Trace trace;

         ASSERT_TRUE( environment::variable::exists( "HOME"));

         auto home = environment::variable::get( "HOME");

         auto result =  environment::expand( "${HOME}/a/b/c");

         EXPECT_TRUE( result == home + "/a/b/c") << "result: " << result;
      }

      TEST( common_environment, environment_string__variable_in_middle__expect_altered)
      {
         common::unittest::Trace trace;

         ASSERT_TRUE( environment::variable::exists( "HOME"));

         auto home = environment::variable::get( "HOME");

         auto result =  environment::expand( "a/b/c/${HOME}/a/b/c");

         EXPECT_TRUE( result == "a/b/c/" + home + "/a/b/c") << "result: " << result;
      }

      TEST( common_environment, environment_file_path__variable_in_middle__expect_altered)
      {
         common::unittest::Trace trace;

         ASSERT_TRUE( environment::variable::exists( "HOME"));

         auto home = environment::variable::get( "HOME");

         auto result =  environment::expand( std::filesystem::path{ "a/b/c/${HOME}/a/b/c"});

         EXPECT_TRUE( result == "a/b/c/" + home + "/a/b/c") << "result: " << result;
      }

      TEST( common_environment, string_variable__expect_altered)
      {
         common::unittest::Trace trace;

         ASSERT_TRUE( environment::variable::exists( "HOME"));

         auto home = environment::variable::get( "HOME");

         auto result =  environment::expand( "${HOME}/a/b/c");

         EXPECT_TRUE( result == home + "/a/b/c") << "result: " << result;
      }

      TEST( common_environment, string_variable__not_correct_format__expect_no_alteration)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( environment::expand( "${HOME/a/b/c") == "${HOME/a/b/c");
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

         auto result =  environment::expand( "main.cpp -I${CASUAL_HOME}/include");

         EXPECT_TRUE( result == "main.cpp -Itest/include") << "result: " << result;
      }

      TEST( common_environment, environment_CASUAL_HOME__variable_before_dash_I__expect_altered)
      {
         common::unittest::Trace trace;

         environment::variable::set( "CASUAL_HOME", "test");

         auto result =  environment::expand( "-I${CASUAL_HOME}/include");

         EXPECT_TRUE( result == "-Itest/include") << "result: " << result;
      }

      TEST( common_environment, local_repository)
      {
         common::unittest::Trace trace;

         std::vector< environment::Variable> local{ { "FOO=foo"}, {"BAR=bar"}};

         auto result = environment::expand( "${FOO}${BAR}", local);

         EXPECT_TRUE( result == "foobar") << "result: " << result;
      }

      TEST( common_environment, local_repository_complex)
      {
         common::unittest::Trace trace;

         std::vector< environment::Variable> local{ { "FOO=foo"}, {"BAR=bar"}};

         auto result = environment::expand( "a${FOO}b${BAR}c", local);

         EXPECT_TRUE( result == "afoobbarc") << "result: " << result;
      }

      TEST( common_environment, local_repository__fallback_environment)
      {
         common::unittest::Trace trace;

         environment::variable::set( "POOP", "poop");

         std::vector< environment::Variable> local{ { "FOO=foo"}, {"BAR=bar"}};

      
         auto result = environment::expand( "${FOO}${POOP}${BAR}", local);

         EXPECT_TRUE( result == "foopoopbar") << "result: " << result;
      }  

   } // common
} // casual




