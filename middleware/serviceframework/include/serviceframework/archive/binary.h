//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "serviceframework/archive/archive.h"
#include "common/platform.h"

namespace casual
{
   namespace serviceframework
   {
      namespace archive
      {
         namespace binary
         {


            archive::Reader reader( const common::platform::binary::type& destination);

            archive::Writer writer( common::platform::binary::type& destination);

         } // binary
      } // archive
   } // serviceframework
} // casual


