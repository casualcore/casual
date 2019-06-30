//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/serialize/archive.h"
#include "common/platform.h"

#include <string>
#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace ini
         {
            namespace strict
            {
               serialize::Reader reader( const std::string& source);
               serialize::Reader reader( std::istream& source);
               serialize::Reader reader( const common::platform::binary::type& source);
            } // strict

            namespace relaxed
            {
               serialize::Reader reader( const std::string& source);
               serialize::Reader reader( std::istream& source);
               serialize::Reader reader( const common::platform::binary::type& source);
            } // relaxed

            serialize::Writer writer( std::string& destination);
            serialize::Writer writer( std::ostream& destination);
            serialize::Writer writer( common::platform::binary::type& destination);

         } // ini
      } // serialize
   } // common
} // casual





