//!
//! test_broker_transformation.h
//!
//! Created on: Jul 9, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "broker/transformation.h"
#include "broker/broker.h"
#include "broker/servervo.h"

namespace casual
{
   namespace broker
   {


      TEST( casual_broker_transformation, server_to_serverVO)
      {
         auto server = std::make_shared< broker::Server>();
         server->path = "a/b/c";
         server->pid = 10;
         server->queue_id = 666;

         //auto transformer = transform::Chain::link( transform::Server());
         admin::transform::Server transformer;

         admin::ServerVO result = transformer( server);


         EXPECT_TRUE( result.getPid() == 10) << "result.getPid(): " << result.getPid();
         EXPECT_TRUE( result.getPath() == "a/b/c");
         EXPECT_TRUE( result.getQueue() == 666);
         EXPECT_TRUE( result.getIdle() == true);

      }

      TEST( casual_broker_transformation, set_server_to_serverVO)
      {
         auto server = std::make_shared< broker::Server>();
         server->path = "a/b/c";
         server->pid = 10;
         server->queue_id = 666;

         State::server_mapping_type serverMapping{ { 10, server}};

         std::vector< admin::ServerVO> result;

         std::transform(
               std::begin( serverMapping),
               std::end( serverMapping),
               std::back_inserter( result),
                  admin::transform::Chain::link(
                     admin::transform::Server(),
                     sf::functional::extract::Second()));

         ASSERT_TRUE( result.size() == 1);
         EXPECT_TRUE( result.at( 0).getPid() == 10) << "result.at( 0).getPid(): " << result.at( 0).getPid();
         EXPECT_TRUE( result.at( 0).getPath() == "a/b/c");
         EXPECT_TRUE( result.at( 0).getQueue() == 666);
         EXPECT_TRUE( result.at( 0).getIdle() == true);

      }


   }


}

