//!
//! casual 
//!

#include <gtest/gtest.h>


#include "domain/manager/state.h"

namespace casual
{
   namespace domain
   {
      namespace manager
      {
         TEST( casual_domain_state_boot_order, empty_state___expect_empty_boot_order)
         {
            State state;

            EXPECT_TRUE( state.bootorder().empty());

         }

         TEST( casual_domain_state_boot_order, executable_1___expect_1_boot_order)
         {
            State state;

            //state.groups;


            EXPECT_TRUE( state.bootorder().empty());

         }


      } // manager

   } // domain


} // casual
