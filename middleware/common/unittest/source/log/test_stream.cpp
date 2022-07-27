//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/unittest/file.h"

#include "common/log/stream.h"
#include "common/log/category.h"
#include "common/execute.h"
#include "common/environment.h"

namespace casual
{
   namespace common
   {
      namespace local
      {
         namespace
         {
            namespace log::scoped
            {
               auto path( std::filesystem::path path)
               {
                  auto update_path = []( auto path)
                  {
                     common::log::stream::Configure configure;
                     configure.path = std::move( path);

                     common::log::stream::configure( configure);
                  };
                  
                  auto origin = common::log::stream::path();
                  update_path( std::move( path));

                  return execute::scope( [ update_path, origin = std::move( origin)]()
                  {
                     update_path( origin);
                  });
               }

               auto inclusive( std::string_view expression)
               {
                  auto update_inclusive = []( auto expression)
                  {
                     common::log::stream::Configure configure;
                     configure.expression.inclusive = expression;

                     common::log::stream::configure( configure);
                  };

                  update_inclusive( expression);

                  return execute::scope( [ update_inclusive]()
                  {
                     update_inclusive( common::environment::variable::get( common::environment::variable::name::log::pattern, ""));
                  });

               }
            } // log::scoped

            
         } // <unnamed>
      } // local

      TEST( common_log_stream, relocate__expect_different_path)
      {
         unittest::Trace trace;

         const auto origin = log::stream::path();
         const auto path = unittest::file::temporary::name( "casual.log");
         
         auto scope = local::log::scoped::path( path);

         EXPECT_TRUE( path == log::stream::path());
         EXPECT_TRUE( origin != log::stream::path()) << trace.compose( "origin: ", origin, ", path: ", log::stream::path());
      }

      TEST( common_log_stream, relocate__expect_content_in_new_location)
      {
         unittest::Trace trace;

         const auto path = unittest::file::temporary::name( "casual.log");

         auto scope = local::log::scoped::path( path);

         log::stream::activate( "casual.common.verbose");

         constexpr std::string_view token{ "c1e2b63a842049c496b0f476e5401621"};
         
         log::line( verbose::log, token);

         // verify that we got the log to the new location.
         const auto content = unittest::file::fetch::content( path);

         EXPECT_TRUE( algorithm::search( content, token)) << "content: " << content;
      }

      TEST( common_log_stream, expression_inclusive__empty_string__expect_no_log)
      {
         unittest::Trace trace;

         auto scoped_inclusive = local::log::scoped::inclusive( "");

         const auto path = unittest::file::temporary::name( "casual.log");
         auto scoped_path = local::log::scoped::path( path);

         log::line( log::category::information, "some logged line");

         EXPECT_TRUE( unittest::file::empty( path));

         // error should always log
         log::line( log::category::error, "some logged line");

         EXPECT_TRUE( ! unittest::file::empty( path));
      }

      TEST( common_log_stream, expression_inclusive__casual_common_verbose__expect_others_to_be_disabled)
      {
         unittest::Trace trace;

         auto scoped_inclusive = local::log::scoped::inclusive( "casual.common.verbose");

         const auto path = unittest::file::temporary::name( "casual.log");
         auto scoped_path = local::log::scoped::path( path);

         log::line( log::category::information, "some logged line");
         EXPECT_TRUE( unittest::file::empty( path));

         log::line( log::debug, "some logged line");
         EXPECT_TRUE( unittest::file::empty( path));

         log::line( verbose::log, "some logged line");
         EXPECT_TRUE( ! unittest::file::empty( path));
      }
      
   } // common
} // casual