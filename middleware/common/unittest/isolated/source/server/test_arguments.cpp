//!
//! casual 
//!

#include "common/unittest.h"

#include "common/server/argument.h"


namespace casual
{
   namespace common
   {
      namespace local
      {
         namespace
         {
            void service1( TPSVCINFO *) {}
            void service3( TPSVCINFO *, int) {}
            void service4( TPSVCINFO *, std::string& value) { value = "test";}
         } // <unnamed>
      } // local



      TEST( common_server_argument, construction)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            server::Arguments arguments{ {}};
         });
      }

      TEST( common_server_argument, service_emplace_back)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            server::Arguments arguments{ {}};

            arguments.services.emplace_back( ".1", &local::service1);
         });
      }


      TEST( common_server_argument, bind_service_emplace_back)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            server::Arguments arguments{ {}};

            arguments.services.emplace_back( ".1", std::bind( &local::service3, std::placeholders::_1, 10));
         });
      }

      TEST( common_server_argument, bind_service_type_trans_emplace_back)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            server::Arguments arguments{ {}};

            arguments.services.emplace_back( ".1", std::bind( &local::service3, std::placeholders::_1, 10), common::service::category::admin, common::service::transaction::Type::none);
         });
      }




      TEST( common_server_argument, bind_ref_service_type_trans_emplace_back)
      {
         common::unittest::Trace trace;

         std::string value;

         EXPECT_NO_THROW({
            server::Arguments arguments{ {}};
            arguments.services.emplace_back( ".1", std::bind( &local::service4, std::placeholders::_1, std::ref( value)), common::service::category::admin, common::service::transaction::Type::none);
            arguments.services.back().call( nullptr);
         });

         EXPECT_TRUE( value == "test");

      }

      namespace local
      {
         namespace
         {
            server::Arguments make_arguments( std::string& value)
            {
               server::Arguments arguments{ {}};
               arguments.services.emplace_back( ".1", std::bind( &local::service4, std::placeholders::_1, std::ref( value)), common::service::category::admin, common::service::transaction::Type::none);
               arguments.services.back().call( nullptr);

               return arguments;
            }
         } // <unnamed>
      } // local


      TEST( common_server_argument, bind_ref_service_type_trans_emplace_back__return_by_value)
      {
         common::unittest::Trace trace;

         std::string value;

         auto arguments = local::make_arguments( value);

         EXPECT_TRUE( value == "test");
      }


   } // common
} // casual
