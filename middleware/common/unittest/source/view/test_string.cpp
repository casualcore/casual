//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "common/view/string.h" 

namespace casual
{
   namespace common
   {
      namespace view 
      {
         
        
         TEST( common_view_string, make_from_string_literal__expect_exact_size)
         {
            common::unittest::Trace trace;

            {
               view::String range( "1");
               EXPECT_TRUE( range.size() == 1);
            }
            {
               view::String range( "12");
               EXPECT_TRUE( range.size() == 2);
            }
            {
               view::String range( "123");
               EXPECT_TRUE( range.size() == 3);
            }
         }    

      } // view 
   } // common
   
} // casual