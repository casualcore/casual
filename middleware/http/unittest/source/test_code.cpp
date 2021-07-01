//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "http/code.h"

#include "common/code/category.h"

namespace casual
{
   using namespace common;

   namespace http
   {

      TEST( http_code, is_category_http)
      {
         unittest::Trace trace;

         auto code = std::error_code{ http::code::not_found};

         EXPECT_TRUE( common::code::is::category< http::code>( code));
      }
      
   } // http
   
} // casual