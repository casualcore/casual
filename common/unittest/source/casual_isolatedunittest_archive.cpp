//!
//! casual_isolatedunittest_archive.cpp
//!
//! Created on: Jun 9, 2012
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "casual_archive.h"
#include "casual_message.h"

namespace casual
{
	namespace archive
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

			input::Binary input( output.get());

			long resultLong;
			input >> resultLong;
			EXPECT_TRUE( resultLong == someLong) << resultLong;

			std::string resultString;
			input >> resultString;
			EXPECT_TRUE( resultString == someString) << resultString;

		}

		TEST( casual_common, archive_io)
		{

			message::ServerConnect serverConnect;

			serverConnect.queue_key = 666;
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

			input::Binary input( output.get());

			message::ServerConnect result;

			input >> result;

			EXPECT_TRUE( result.queue_key == 666) << result.queue_key;
			EXPECT_TRUE( result.serverPath == "/bla/bla/bla/sever") << result.serverPath;
			EXPECT_TRUE( result.services.size() == 3) << result.services.size();

		}


		TEST( casual_common, archive_io_big_size)
		{

			message::ServerConnect serverConnect;

			serverConnect.queue_key = 666;
			serverConnect.serverPath = "/bla/bla/bla/sever";


			message::Service service;
			service.name = "service1";
			serverConnect.services.resize( 10000, service);

			output::Binary output;

			output << serverConnect;

			input::Binary input( output.get());

			message::ServerConnect result;

			input >> result;

			EXPECT_TRUE( result.queue_key == 666) << result.queue_key;
			EXPECT_TRUE( result.serverPath == "/bla/bla/bla/sever") << result.serverPath;
			EXPECT_TRUE( result.services.size() == 10000) << result.services.size();

		}



	}


}



