//!
//! casual_isolatedunittest_broker_confguration.cpp
//!
//! Created on: Nov 5, 2012
//!     Author: Lazan
//!



#include <gtest/gtest.h>

#include "broker/state.h"
#include "broker/broker.h"
#include "broker/transform.h"

#include "sf/log.h"


#include "config/domain.h"

#include <iosfwd>



namespace casual
{
   namespace broker
   {
      TEST( casual_broker_configuration, transform_domain)
      {

         auto domain = config::domain::get( common::file::basedir( __FILE__) + "/../../../../configuration/domain.yaml");


         auto state = transform::configuration::Domain{}( domain);

         ASSERT_TRUE( state.groups.size() == 6) << CASUAL_MAKE_NVP( state.groups);


         EXPECT_TRUE( state.groups.at( 1).dependencies.size() == 1) << CASUAL_MAKE_NVP(state.groups.at( 1));

         ASSERT_TRUE( state.groups.at( 2).dependencies.size() == 1) << CASUAL_MAKE_NVP(state.groups.at( 2));
         EXPECT_TRUE( state.groups.at( 2).dependencies.at( 0) == 10) << CASUAL_MAKE_NVP(state.groups.at( 2));

      }

      /*
      TEST( casual_broker_configuration, boot_order)
      {
         State state;

         auto domain = config::domain::get( common::file::basedir( __FILE__) + "../../../../configuration/domain.yaml");
         action::add::groups( state, domain.groups);

         auto bootOrder = action::bootOrder( state);

         ASSERT_TRUE( bootOrder.size() == 3) << "sections: " << bootOrder.size();
         EXPECT_TRUE( bootOrder.at( 0).size() == 3);
         EXPECT_TRUE( bootOrder.at( 1).size() == 1);
         EXPECT_TRUE( bootOrder.at( 2).size() == 1);


      }
      */


   }
}




