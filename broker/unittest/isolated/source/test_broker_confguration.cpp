//!
//! casual_isolatedunittest_broker_confguration.cpp
//!
//! Created on: Nov 5, 2012
//!     Author: Lazan
//!



#include <gtest/gtest.h>

#include "broker/broker.h"
#include "broker/action.h"


#include "config/domain.h"

#include <iosfwd>



namespace casual
{
   namespace broker
   {
      TEST( casual_broker_configuration, add_groups)
      {
         State state;

         auto domain = config::domain::get();

         action::addGroups( state, domain);

         EXPECT_TRUE( state.groups.size() == 5);


         EXPECT_TRUE( state.groups.at( "group3")->dependencies.size() == 1) << "size: " << state.groups.at( "group3")->dependencies.size();

         ASSERT_TRUE( state.groups.at( "group4")->dependencies.size() == 2);
         EXPECT_TRUE( state.groups.at( "group4")->dependencies.at( 0)->name == "group3");
         EXPECT_TRUE( state.groups.at( "group4")->dependencies.at( 1)->name == "casual");


      }

      TEST( casual_broker_configuration, boot_order)
      {
         State state;

         auto domain = config::domain::get();
         action::addGroups( state, domain);

         auto bootOrder = action::bootOrder( state);

         ASSERT_TRUE( bootOrder.size() == 3) << "sections: " << bootOrder.size();
         EXPECT_TRUE( bootOrder.at( 0).size() == 3);
         EXPECT_TRUE( bootOrder.at( 1).size() == 1);
         EXPECT_TRUE( bootOrder.at( 2).size() == 1);


      }


   }
}




