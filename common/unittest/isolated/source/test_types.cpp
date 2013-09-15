//!
//! test_types.cpp
//!
//! Created on: Aug 29, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "common/types.h"

namespace casual
{

   namespace common
   {
      TEST( casual_common_types, xid_equal_to_nullptr)
      {
         XID xid;
         xid.formatID = common::cNull_XID;

         EXPECT_TRUE( xid == nullptr);
         EXPECT_TRUE( nullptr == xid);
      }
   } // common
}
