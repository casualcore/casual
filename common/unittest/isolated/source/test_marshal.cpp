//!
//! casual_isolatedunittest_archive.cpp
//!
//! Created on: Jun 9, 2012
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "common/marshal.h"
#include "common/message.h"


#include "common/types.h"

namespace casual
{
   namespace common
   {
      namespace marshal
      {

         /*
         void print( const std::vector< char>& buffer)
         {
            std::vector< char>::const_iterator current = buffer.begin();

            for(; current != buffer.end(); ++current)
            {
               if( *current == '\0')
               {
                  std::cout << "0";
               }
               else
               {
                  std::cout << static_cast< int>( *current);
               }
            }
            std::cout << std::endl;
         }
         */


         TEST( casual_common, archive_basic_io)
         {
            long someLong = 3;
            std::string someString = "banan";

            output::Binary output;

            output << someLong;

            EXPECT_TRUE( output.get().size() == sizeof( long)) <<  output.get().size();

            output << someString;

            input::Binary input( std::move( output));

            long resultLong;
            input >> resultLong;
            EXPECT_TRUE( resultLong == someLong) << resultLong;

            std::string resultString;
            input >> resultString;
            EXPECT_TRUE( resultString == someString) << resultString;

         }

         TEST( casual_common, archive_binary)
         {
            std::vector< char> binaryInput;

            for( int index = 0; index < 3000; ++index)
            {
               binaryInput.push_back( static_cast< char>( index));

            }

            output::Binary output;
            output << binaryInput;
            EXPECT_TRUE( output.get().size() == binaryInput.size() + sizeof( binaryInput.size())) <<  output.get().size();


            input::Binary input( std::move( output));

            std::vector< char> binaryOutput;
            input >> binaryOutput;

            EXPECT_TRUE( binaryInput == binaryOutput);
         }

         TEST( casual_common, archive_io)
         {

            message::service::Advertise serverConnect;

            serverConnect.server.queue_id = 666;
            serverConnect.serverPath = "/bla/bla/bla/sever";

            message::Service service;

            service.name = "service1";
            serverConnect.services.push_back( service);

            service.name = "service2";
            serverConnect.services.push_back( service);

            service.name = "service3";
            serverConnect.services.push_back( service);



            output::Binary output;

            output << serverConnect;

            input::Binary input( std::move( output));

            message::service::Advertise result;

            input >> result;

            EXPECT_TRUE( result.server.queue_id == 666) << result.server.queue_id;
            EXPECT_TRUE( result.serverPath == "/bla/bla/bla/sever") << result.serverPath;
            EXPECT_TRUE( result.services.size() == 3) << result.services.size();

         }


         TEST( casual_common, archive_io_big_size)
         {

            message::service::Advertise serverConnect;

            serverConnect.server.queue_id = 666;
            serverConnect.serverPath = "/bla/bla/bla/sever";


            message::Service service;
            service.name = "service1";
            serverConnect.services.resize( 10000, service);

            output::Binary output;

            output << serverConnect;

            input::Binary input( std::move( output));

            message::service::Advertise result;

            input >> result;

            EXPECT_TRUE( result.server.queue_id == 666) << result.server.queue_id;
            EXPECT_TRUE( result.serverPath == "/bla/bla/bla/sever") << result.serverPath;
            EXPECT_TRUE( result.services.size() == 10000) << result.services.size();

         }

         TEST( casual_common, marshal_null_XID)
         {

            XID xid_source;

            xid_source.formatID = common::cNull_XID;

            output::Binary output;

            output << xid_source;

            input::Binary input( std::move( output));

            XID xid_target;

            input >> xid_target;

            EXPECT_TRUE( xid_target.formatID == cNull_XID);
         }

         TEST( casual_common, marshal_XID)
         {

            XID xid_source;

            auto uuid_source = Uuid::make();
            long uuid_size = sizeof( decltype( uuid_source.get()));

            xid_source.formatID = 1;
            xid_source.gtrid_length = uuid_size;
            xid_source.bqual_length = uuid_size;

            std::copy( std::begin(uuid_source.get()), std::end( uuid_source.get()), xid_source.data);
            std::copy( std::begin(uuid_source.get()), std::end( uuid_source.get()), xid_source.data + uuid_size);

            output::Binary output;

            output << xid_source;

            input::Binary input( std::move( output));

            XID xid_target;

            input >> xid_target;

            ASSERT_TRUE( xid_target.gtrid_length == uuid_size);
            ASSERT_TRUE( xid_target.bqual_length == uuid_size);


            auto gtrid_start = std::begin( xid_target.data);
            auto gtrid_end = std::begin( xid_target.data) + xid_target.gtrid_length;
            auto bqual_start = gtrid_end;
            auto bqual_end = bqual_start + xid_target.bqual_length;

            Uuid gtrid;
            std::copy( gtrid_start, gtrid_end, std::begin( gtrid.get()));

            Uuid bqual;
            std::copy( bqual_start, bqual_end, std::begin( bqual.get()));

            EXPECT_TRUE( gtrid == uuid_source);
            EXPECT_TRUE( bqual == uuid_source);
         }

      }

   }
}



