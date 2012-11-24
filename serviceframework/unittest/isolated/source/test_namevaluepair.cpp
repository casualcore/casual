/*
 * casual_isolatedunittest_namevaluepair.cpp
 *
 *  Created on: Sep 16, 2012
 *      Author: lazan
 */


#include <gtest/gtest.h>


#include "sf/namevaluepair.h"



namespace casual
{
	TEST( casual_sf_NameValuePair, instansiation)
	{
		long someLong = 10;

		auto nvp = CASUAL_MAKE_NVP( someLong);

		EXPECT_TRUE( nvp.getName() == std::string( "someLong"));
		EXPECT_TRUE( nvp.getConstValue() == 10);
		EXPECT_TRUE( nvp.getValue() == 10);
	}

	TEST( casual_sf_NameValuePair, instansiation_const)
	{
		const long someLong = 10;

		EXPECT_TRUE( CASUAL_MAKE_NVP( someLong).getName() == std::string( "someLong"));
		EXPECT_TRUE( CASUAL_MAKE_NVP( someLong).getConstValue() == 10);

	}

	namespace local
	{
		long getLong( long value) { return value;}
	}

	TEST( casual_sf_NameValuePair, instansiation_rvalue)
	{
		EXPECT_TRUE( CASUAL_MAKE_NVP( 10L).getName() == std::string( "10L"));
		EXPECT_TRUE( CASUAL_MAKE_NVP( 10L).getConstValue() == 10);
	}


}
