//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>
#include "common/unittest.h"

#include "configuration/environment.h"
#include "configuration/common.h"

#include "serviceframework/archive/create.h"
#include "serviceframework/log.h"

#include <fstream>

namespace casual
{
   namespace configuration
   {


      TEST( configuration_environment_fetch, empty_files__expect_none)
      {
         Environment empty;

         auto result = environment::fetch( empty);

         EXPECT_TRUE( result.empty());
      }

      TEST( configuration_environment_fetch, empty_files__some_variables__expect_variables)
      {
         Environment env;
         env.variables = {
               { []( environment::Variable& v){
                  v.key = "a";
                  v.value = "b";
               }},
               { []( environment::Variable& v){
                  v.key = "c";
                  v.value = "d";
               }}
         };

         auto result = environment::fetch( env);

         EXPECT_TRUE( result == env.variables);
      }


      class configuration_environment : public ::testing::TestWithParam< const char*>
      {
      };


      INSTANTIATE_TEST_CASE_P( protocol,
            configuration_environment,
         ::testing::Values(".yaml", ".json", ".xml", ".ini"));


      namespace local
      {
         namespace
         {

            common::file::scoped::Path serialize( const Environment& environment, const std::string& extension)
            {
               common::file::scoped::Path name{ common::file::name::unique( common::directory::temporary() + "/domain", extension)};
               
               common::file::Output file{ name};
               auto archive = serviceframework::archive::create::writer::from( file.extension(), file);
               
               archive << CASUAL_MAKE_NVP( environment);

               return name;
            }

            std::vector< environment::Variable> variables( int size = 8)
            {
               static int count = 1;

               std::vector< environment::Variable> result( size);

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

         Environment origin;
         origin.variables = local::variables();

         auto env_file = local::serialize( origin, GetParam());

         Environment refered;
         refered.files.push_back( env_file);

         auto result = environment::fetch( refered);

         //env_file.release();

         EXPECT_TRUE( result == origin.variables) << "result: " << CASUAL_MAKE_NVP( result) << "refered: " << CASUAL_MAKE_NVP( refered);
      }


      TEST_P( configuration_environment, file_referenced__added_variables__expect__added_to_be_last)
      {
         common::unittest::Trace trace;

         Environment origin;
         origin.variables = local::variables();

         auto env_file = local::serialize( origin, GetParam());

         Environment refered;
         {
            refered.files.push_back( env_file);
            refered.variables = {
                  { []( environment::Variable& v){
                     v.key = "k_c";
                     v.value = "v_c";
                  }},
                  { []( environment::Variable& v){
                     v.key = "k_d";
                     v.value = "v_d";
                  }}
            };
         }

         auto result = environment::fetch( refered);

         EXPECT_TRUE( result.size() == refered.variables.size() + origin.variables.size());
         EXPECT_TRUE( result.back() ==  refered.variables.back());
      }

      TEST_P( configuration_environment, hierarchy_2_files__3_env___expect__filo_order)
      {
         common::unittest::Trace trace;

         Environment first;
         first.variables = local::variables();
         auto first_file = local::serialize( first, GetParam());

         configuration::log << CASUAL_MAKE_NVP( first);

         Environment second;
         second.variables = local::variables();
         second.files.push_back( first_file);
         auto second_file = local::serialize( second, GetParam());

         configuration::log << CASUAL_MAKE_NVP( second);

         Environment third;
         third.variables = local::variables();
         third.files.push_back( second_file);

         configuration::log << CASUAL_MAKE_NVP( third);


         auto expected = first.variables;
         {
            common::algorithm::append( second.variables, expected);
            common::algorithm::append( third.variables, expected);
         }


         auto result = environment::fetch( third);

         EXPECT_TRUE( result == expected) << CASUAL_MAKE_NVP( result);
      }



   } // configuration

} // casual
