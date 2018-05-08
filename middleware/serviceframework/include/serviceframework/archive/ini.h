//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "serviceframework/archive/archive.h"

#include "common/platform.h"
#include <string>
#include <iosfwd>

namespace casual
{
   namespace serviceframework
   {
      namespace archive
      {
         namespace ini
         {
            namespace relaxed
            {
               archive::Reader reader( const std::string& source);
               archive::Reader reader( std::istream& source);
               archive::Reader reader( const common::platform::binary::type& source);
            }

            archive::Reader reader( const std::string& source);
            archive::Reader reader( std::istream& source);
            archive::Reader reader( const common::platform::binary::type& source);


            archive::Writer writer( std::string& destination);
            archive::Writer writer( std::ostream& destination);
            archive::Writer writer( common::platform::binary::type& destination);

         } // ini
      } // archive
   } // serviceframework
} // casual





