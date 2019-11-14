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
         namespace yaml
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
            }

            namespace consumed
            {    
               serialize::Reader reader( const std::string& source);
               serialize::Reader reader( std::istream& source);
               serialize::Reader reader( const platform::binary::type& source);
            }

            serialize::Writer writer( std::string& destination);
            serialize::Writer writer( std::ostream& destination);
            serialize::Writer writer( platform::binary::type& destination);

         } // yaml
      } // serialize
   } // common
} // casual




