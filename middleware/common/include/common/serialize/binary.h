//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/serialize/archive.h"
#include "casual/platform.h"

namespace casual
{
   namespace common::serialize::binary
   {
      serialize::Reader reader( const platform::binary::type& source);
      serialize::Writer writer();
      
   } // common::serialize::binary
} // casual


