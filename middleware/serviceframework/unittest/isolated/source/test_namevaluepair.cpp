//!
//! casual
//!

#include <gtest/gtest.h>


#include "sf/namevaluepair.h"



namespace casual
{
	TEST( casual_sf_NameValuePair, instansiation)
	{
		long someLong = 10;

		auto nvp = CASUAL_MAKE_NVP( someLong);

		EXPECT_TRUE( nvp.name() == std::string( "someLong"));
		EXPECT_TRUE( nvp.value() == 10);
	}

	TEST( casual_sf_NameValuePair, instansiation_const)
	{
		const long someLong = 10;

		EXPECT_TRUE( CASUAL_MAKE_NVP( someLong).name() == std::string( "someLong"));
		EXPECT_TRUE( CASUAL_MAKE_NVP( someLong).value() == 10);

	}

	namespace local
	{
		long getLong( long value) { return value;}
	}

	TEST( casual_sf_NameValuePair, instansiation_rvalue)
	{
		EXPECT_TRUE( CASUAL_MAKE_NVP( 10L).name() == std::string( "10L"));
		EXPECT_TRUE( CASUAL_MAKE_NVP( 10L).value() == 10);
	}


}
