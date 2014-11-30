//!
//! test_broker_transformation.h
//!
//! Created on: Jul 9, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "broker/transformation.h"
#include "broker/broker.h"
#include "broker/brokervo.h"
#include "../../../../serviceframework/include/sf/archive/binary.h"
#include "../../../../serviceframework/include/sf/buffer.h"
#include "../../../admin/include/broker/brokervo.h"

namespace casual
{
   namespace broker
   {
      struct Nested
      {
         std::string pid;
         std::string queueId;
         std::string state;

         template< typename A>
         void serialize( A& archive)
         {
            archive & CASUAL_MAKE_NVP( pid);
            archive & CASUAL_MAKE_NVP( queueId);
            archive & CASUAL_MAKE_NVP( state);
         }
      };

      struct Composite
      {
         std::string alias;
         std::string path;
         std::vector< Nested> instances;

         template< typename A>
         void serialize( A& archive)
         {
            archive & CASUAL_MAKE_NVP( alias);
            archive & CASUAL_MAKE_NVP( path);
            archive & CASUAL_MAKE_NVP( instances);
         }
      };

      /*

      TEST( casual_broker_transformation, server_to_serverVO)
      {
         sf::buffer::Binary buffer;

         {
            admin::ServerVO serverVO;
            serverVO.alias = "test";
            serverVO.instances = { { 1, 1, 1}, {2, 2, 2}};


            sf::archive::binary::Writer writer( buffer);
            writer << CASUAL_MAKE_NVP( serverVO);
         }

         {
            sf::archive::binary::Reader reader( buffer);

            admin::ServerVO serverVO;
            reader >> CASUAL_MAKE_NVP( serverVO);

            EXPECT_TRUE( serverVO.alias == "test");
            ASSERT_TRUE( serverVO.instances.size() == 2);
            EXPECT_TRUE( serverVO.instances.at( 0).pid == 1);
            EXPECT_TRUE( serverVO.instances.at( 1).pid == 2);
         }

      }
      */

      TEST( casual_broker_transformation, server_to_nested)
      {
         sf::buffer::binary::Stream buffer;

         {
            Composite composite;
            composite.alias = "test";
            composite.instances = { { "1", "1", "1"}, {"2", "2", "2"}};


            sf::archive::binary::Writer writer( buffer);
            writer << CASUAL_MAKE_NVP( composite);
         }

         {
            sf::archive::binary::Reader reader( buffer);

            Composite composite;
            reader >> CASUAL_MAKE_NVP( composite);

            EXPECT_TRUE( composite.alias == "test");
            ASSERT_TRUE( composite.instances.size() == 2);
            EXPECT_TRUE( composite.instances.at( 0).pid == "1");
            EXPECT_TRUE( composite.instances.at( 1).pid == "2");
         }

      }

      /*

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

*/

   }


}

