//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/serialize/archive.h"
#include "casual/platform.h"

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
               serialize::Reader reader( const std::string& source);
               serialize::Reader reader( std::istream& source);
               serialize::Reader reader( const platform::binary::type& source);
            } // strict 

            namespace relaxed
            {    
               serialize::Reader reader( const std::string& source);
               serialize::Reader reader( std::istream& source);
               serialize::Reader reader( const platform::binary::type& source);
            } // relaxed

            namespace consumed
            {    
               serialize::Reader reader( const std::string& source);
               serialize::Reader reader( std::istream& source);
               serialize::Reader reader( const platform::binary::type& source);
            } // consumed

            namespace pretty
            {
               serialize::Writer writer();
            } // pretty

            serialize::Writer writer();

         } // json
      } // serialize
   } // common
} // casual





