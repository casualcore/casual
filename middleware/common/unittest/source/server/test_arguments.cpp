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

            namespace lifetime
            {
               struct Init
               {
                  int operator () ( int argc, char **argv) { return 42; }
               };

               struct Done
               {
                  void operator () () {}
               };

            } // lifetime

            service::invoke::Result service1( service::invoke::Parameter&&) { return {};}
            service::invoke::Result service3( service::invoke::Parameter&&, int) { return {};}
            service::invoke::Result service4( service::invoke::Parameter&&, std::string& value) { value = "test"; return {};}
         } // <unnamed>
      } // local



      TEST( common_server_argument, construction)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            server::Arguments arguments( {}, local::lifetime::Init{}, local::lifetime::Done{});
         });
      }

      TEST( common_server_argument, service_emplace_back)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            server::Arguments arguments( {}, local::lifetime::Init{}, local::lifetime::Done{});

            arguments.services.emplace_back( ".1", &local::service1);

         });
      }

      TEST( common_server_argument, server_init)
      {
         common::unittest::Trace trace;
         server::Arguments arguments{ {}, local::lifetime::Init{}, local::lifetime::Done{}};

         EXPECT_TRUE( arguments.init( 0, nullptr) == 42);
      }

      TEST( common_server_argument, server_done)
      {
         common::unittest::Trace trace;
         server::Arguments arguments{ {}, local::lifetime::Init{}, local::lifetime::Done{}};

         EXPECT_NO_THROW({
            arguments.done();
         });
      }


      TEST( common_server_argument, bind_service_emplace_back)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            server::Arguments arguments( {}, local::lifetime::Init{}, local::lifetime::Done{});

            arguments.services.emplace_back( ".1", std::bind( &local::service3, std::placeholders::_1, 10));
         });
      }

      TEST( common_server_argument, bind_service_type_trans_emplace_back)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            server::Arguments arguments( {}, local::lifetime::Init{}, local::lifetime::Done{});

            arguments.services.emplace_back( ".1",
                  std::bind( &local::service3, std::placeholders::_1, 10),
                  common::service::transaction::Type::none, common::service::category::admin);
         });
      }




      TEST( common_server_argument, bind_ref_service_type_trans_emplace_back)
      {
         common::unittest::Trace trace;

         std::string value;

         EXPECT_NO_THROW({
            server::Arguments arguments( {}, local::lifetime::Init{}, local::lifetime::Done{});

            arguments.services.emplace_back( ".1",
                        std::bind( &local::service4, std::placeholders::_1, std::ref( value)),
                        common::service::transaction::Type::none, common::service::category::admin);

            arguments.services.back()( service::invoke::Parameter{ buffer::Payload{ ".binary/", 128}});
         });

         EXPECT_TRUE( value == "test");

      }

      namespace local
      {
         namespace
         {
            server::Arguments make_arguments( std::string& value)
            {
               server::Arguments arguments( {}, local::lifetime::Init{}, local::lifetime::Done{});

               arguments.services = {
                     { ".1", std::bind( &local::service4, std::placeholders::_1, std::ref( value)), common::service::transaction::Type::none, common::service::category::admin}
               };

               arguments.services.back()( service::invoke::Parameter{});

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
