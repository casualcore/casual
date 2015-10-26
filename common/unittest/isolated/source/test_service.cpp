//!
//! test_service.cpp
//!
//! Created on: Jul 5, 2015
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "common/server/service.h"
#include "common/server/argument.h"
#include "xatmi.h"

namespace casual
{
   namespace common
   {

      void service1( TPSVCINFO *) {}

      void service2( TPSVCINFO *) {}


      TEST( casual_common_server_service, equality)
      {
         server::Service s1{ ".1", &service1};
         server::Service s2{ ".2", &service1};

         EXPECT_TRUE( s1 == s2);
      }

      TEST( casual_common_server_service, in_equality)
      {
         server::Service s1{ ".1", &service1};
         server::Service s2{ ".2", &service2};

         EXPECT_TRUE( s1 != s2);
      }


      void service3( TPSVCINFO *, int) {}

      TEST( casual_common_server_service, bind_argument)
      {
         server::Service s1{ ".1", std::bind( &service3, std::placeholders::_1, 10)};
         server::Service s2{ ".2", std::bind( &service3, std::placeholders::_1, 10)};

         EXPECT_TRUE( s1 == s2);
      }



      TEST( casual_common_server_argument, construction)
      {
         EXPECT_NO_THROW({
            server::Arguments arguments{ {}};
         });
      }

      TEST( casual_common_server_argument, service_emplace_back)
      {
         EXPECT_NO_THROW({
            server::Arguments arguments{ {}};

            arguments.services.emplace_back( ".1", &service1);
         });
      }


      TEST( casual_common_server_argument, bind_service_emplace_back)
      {
         EXPECT_NO_THROW({
            server::Arguments arguments{ {}};

            arguments.services.emplace_back( ".1", std::bind( &service3, std::placeholders::_1, 10));
         });
      }

      TEST( casual_common_server_argument, bind_service_type_trans_emplace_back)
      {
         EXPECT_NO_THROW({
            server::Arguments arguments{ {}};

            arguments.services.emplace_back( ".1", std::bind( &service3, std::placeholders::_1, 10), common::server::Service::Type::cCasualAdmin, common::server::Service::Transaction::none);
         });
      }


      void service4( TPSVCINFO *, std::string& value) { value = "test";}

      TEST( casual_common_server_argument, bind_ref_service_type_trans_emplace_back)
      {
         std::string value;

         EXPECT_NO_THROW({
            server::Arguments arguments{ {}};
            arguments.services.emplace_back( ".1", std::bind( &service4, std::placeholders::_1, std::ref( value)), common::server::Service::Type::cCasualAdmin, common::server::Service::Transaction::none);
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
               arguments.services.emplace_back( ".1", std::bind( &service4, std::placeholders::_1, std::ref( value)), common::server::Service::Type::cCasualAdmin, common::server::Service::Transaction::none);
               arguments.services.back().call( nullptr);

               return arguments;
            }
         } // <unnamed>
      } // local


      TEST( casual_common_server_argument, bind_ref_service_type_trans_emplace_back__return_by_value)
      {
         std::string value;

         auto arguments = local::make_arguments( value);

         EXPECT_TRUE( value == "test");
      }

   } // common

} // casual
