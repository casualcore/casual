//!
//! casual_isolatedunittest_archive.cpp
//!
//! Created on: Jun 9, 2012
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "common/marshal/binary.h"
#include "common/message/server.h"


#include "common/transaction/id.h"

namespace casual
{
   namespace common
   {
      namespace marshal
      {



         TEST( casual_common, marshal_basic_io)
         {
            long someLong = 3;
            std::string someString = "banan";

            platform::binary_type buffer;
            output::Binary output{ buffer};

            output << someLong;

            EXPECT_TRUE( buffer.size() == sizeof( long)) <<  buffer.size();

            output << someString;

            input::Binary input( buffer);

            long resultLong;
            input >> resultLong;
            EXPECT_TRUE( resultLong == someLong) << resultLong;

            std::string resultString;
            input >> resultString;
            EXPECT_TRUE( resultString == someString) << resultString;
         }

         TEST( casual_common, marshal_binary)
         {
            std::vector< char> binaryInput;

            for( int index = 0; index < 3000; ++index)
            {
               binaryInput.push_back( static_cast< char>( index));

            }

            platform::binary_type buffer;
            output::Binary output( buffer);
            output << binaryInput;

            EXPECT_TRUE( buffer.size() == binaryInput.size() + sizeof( binaryInput.size())) <<  buffer.size();

            input::Binary input( buffer);

            std::vector< char> binaryOutput;
            input >> binaryOutput;

            EXPECT_TRUE( binaryInput == binaryOutput);
         }

         TEST( casual_common, marshal_io)
         {

            message::service::Advertise serverConnect;

            serverConnect.process.queue = 666;
            serverConnect.serverPath = "/bla/bla/bla/sever";

            message::Service service;

            service.name = "service1";
            serverConnect.services.push_back( service);

            service.name = "service2";
            serverConnect.services.push_back( service);

            service.name = "service3";
            serverConnect.services.push_back( service);


            ipc::message::Complete complete = marshal::complete( serverConnect);

            message::service::Advertise result;

            complete >> result;

            EXPECT_TRUE( result.process.queue == 666) << result.process.queue;
            EXPECT_TRUE( result.serverPath == "/bla/bla/bla/sever") << result.serverPath;
            EXPECT_TRUE( result.services.size() == 3) << result.services.size();

         }


         TEST( casual_common, marshal_io_big_size)
         {

            message::service::Advertise serverConnect;

            serverConnect.process.queue = 666;
            serverConnect.serverPath = "/bla/bla/bla/sever";


            message::Service service;
            service.name = "service1";
            serverConnect.services.resize( 10000, service);

            ipc::message::Complete complete = marshal::complete( serverConnect);

            message::service::Advertise result;

            complete >> result;

            EXPECT_TRUE( result.process.queue == 666) << result.process.queue;
            EXPECT_TRUE( result.serverPath == "/bla/bla/bla/sever") << result.serverPath;
            EXPECT_TRUE( result.services.size() == 10000) << result.services.size();

         }

         TEST( casual_common, marshal_transaction_id_null)
         {

            transaction::ID xid_source;

            platform::binary_type buffer;
            output::Binary output( buffer);

            output << xid_source;

            input::Binary input( buffer);

            transaction::ID xid_target;

            input >> xid_target;

            EXPECT_TRUE( xid_target.null());
         }

         TEST( casual_common, marshal_transaction_id)
         {

            transaction::ID xid_source = transaction::ID::create();

            platform::binary_type buffer;
            output::Binary output( buffer);

            output & xid_source;

            input::Binary input( buffer);

            transaction::ID xid_target;

            input & xid_target;

            EXPECT_TRUE( ! xid_target.null());
            EXPECT_TRUE( xid_target == xid_source);

         }

      }

   }
}



