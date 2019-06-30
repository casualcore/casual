//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/serialize/archive.h"
#include "common/platform.h"

#include <iosfwd>
#include <string>

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace json
         {
            namespace strict 
            {
               serialize::Reader reader( const std::string& destination);
               serialize::Reader reader( std::istream& destination);
               serialize::Reader reader( const common::platform::binary::type& destination);
            } // strict 

            namespace relaxed
            {    
               serialize::Reader reader( const std::string& destination);
               serialize::Reader reader( std::istream& destination);
               serialize::Reader reader( const common::platform::binary::type& destination);
            } // relaxed

            namespace consumed
            {    
               serialize::Reader reader( const std::string& source);
               serialize::Reader reader( std::istream& source);
               serialize::Reader reader( const common::platform::binary::type& source);
            } // consumed

            serialize::Writer writer( std::string& destination);
            serialize::Writer writer( std::ostream& destination);
            serialize::Writer writer( common::platform::binary::type& destination);

         } // json
      } // serialize
   } // common
} // casual





