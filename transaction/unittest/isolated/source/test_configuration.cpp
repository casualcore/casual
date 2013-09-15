//!
//! test_configuration.cpp
//!
//! Created on: Aug 3, 2013
//!     Author: Lazan
//!


#include <gtest/gtest.h>


#include "transaction/manager.h"


namespace casual
{

   TEST( casual_transaction_configuration, configure_resource_proxies)
   {
      transaction::State state( "unittest_transaction.db");

      transaction::state::configure( state);

      EXPECT_TRUE( state.xaConfig.size() == 2);
   }

} // casual


