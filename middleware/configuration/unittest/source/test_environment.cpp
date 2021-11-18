//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/file.h"

#include "configuration/user/environment.h"
#include "configuration/common.h"

#include "common/serialize/create.h"

#include <fstream>

namespace casual
{
   namespace configuration
   {

      TEST( configuration_environment_fetch, empty_files__expect_none)
      {
         common::unittest::Trace trace;

         user::domain::Environment empty;

         auto result = user::domain::environment::fetch( empty);

         EXPECT_TRUE( result.empty());
      }

      TEST( configuration_environment_fetch, empty_files__some_variables__expect_variables)
      {
         common::unittest::Trace trace;

         user::domain::Environment env;
         env.variables = {
            [](){
               user::domain::environment::Variable v;
               v.key = "a";
               v.value = "b";
               return v;
            }(),
            [](){
               user::domain::environment::Variable v;
               v.key = "c";
               v.value = "d";
               return v;
            }()
         };

         auto result = user::domain::environment::fetch( env);

         EXPECT_TRUE( result == env.variables);
      }


      class configuration_environment : public ::testing::TestWithParam< const char*>
      {
      };


      INSTANTIATE_TEST_SUITE_P( protocol,
            configuration_environment,
         ::testing::Values(".yaml", ".json", ".xml", ".ini"));


      namespace local
      {
         namespace
         {
            common::file::scoped::Path temporary( const std::string& extension)
            {
               const auto prefix = common::directory::temporary() / "domain";
               return { common::file::name::unique( prefix.string(), extension)};
            }

            common::file::scoped::Path serialize( const user::domain::Environment& environment, const std::string& extension)
            {
               common::file::scoped::Path name{ temporary( extension)};
               
               std::ofstream file{ name};
               auto archive = common::serialize::create::writer::from( extension);
               archive << CASUAL_NAMED_VALUE( environment);
               archive.consume( file);

               return name;
            }

            std::vector< user::domain::environment::Variable> variables( int size = 8)
            {
               static int count = 1;

               std::vector< user::domain::environment::Variable> result( size);

               for( auto& variable : result)
               {
                  variable.key = "k_" + std::to_string( count);
                  variable.value = "v_" + std::to_string( count);
                  ++count;
               }

               return result;
            }

         } // <unnamed>
      } // local


      TEST_P( configuration_environment, file_referenced)
      {
         common::unittest::Trace trace;

         user::domain::Environment origin;
         origin.variables = local::variables();

         auto env_file = local::serialize( origin, GetParam());

         user::domain::Environment refered;
         refered.files.emplace().push_back( env_file);

         auto result = user::domain::environment::fetch( refered);

         //env_file.release();

         EXPECT_TRUE( result == origin.variables) << "result: " << CASUAL_NAMED_VALUE( result) << "refered: " << CASUAL_NAMED_VALUE( refered);
      }


      TEST_P( configuration_environment, file_referenced__added_variables__expect__added_to_be_last)
      {
         common::unittest::Trace trace;

         user::domain::Environment origin;
         origin.variables = local::variables();

         auto env_file = local::serialize( origin, GetParam());

         user::domain::Environment refered;
         {
            refered.files.emplace().push_back( env_file);
            refered.variables = {
               [](){
                  user::domain::environment::Variable v;
                  v.key = "k_c";
                  v.value = "v_c";
                  return v;
               }(),
               [](){
                  user::domain::environment::Variable v;
                  v.key = "k_d";
                  v.value = "v_d";
                  return v;
               }()
            };
         }

         auto result = user::domain::environment::fetch( refered);

         EXPECT_TRUE( result.size() == refered.variables.value().size() + origin.variables.value().size());
         //EXPECT_TRUE( result.back() ==  refered.variables.back());
      }

      TEST_P( configuration_environment, hierarchy_2_files__3_env___expect__filo_order)
      {
         common::unittest::Trace trace;

         user::domain::Environment first;
         first.variables = local::variables();
         auto first_file = local::serialize( first, GetParam());

         common::log::line( configuration::log, "first: ", first);

         user::domain::Environment second;
         second.variables = local::variables();
         second.files.emplace().push_back( first_file);
         auto second_file = local::serialize( second, GetParam());

         common::log::line( configuration::log, "second: ", second);

         user::domain::Environment third;
         third.variables = local::variables();
         third.files.emplace().push_back( second_file);

         common::log::line( configuration::log, "third: ", third);


         auto expected = first.variables.value();
         {
            common::algorithm::append( second.variables.value(), expected);
            common::algorithm::append( third.variables.value(), expected);
         }


         auto result = user::domain::environment::fetch( third);

         EXPECT_TRUE( result == expected) << "   " << CASUAL_NAMED_VALUE( result) << "\n" << CASUAL_NAMED_VALUE( expected);
      }



   } // configuration

} // casual
