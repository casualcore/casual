//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef ARCHIVE_JSON_H_
#define ARCHIVE_JSON_H_


#include "serviceframework/archive/archive.h"
#include "serviceframework/platform.h"

#include <iosfwd>
#include <string>

namespace casual
{
   namespace serviceframework
   {
      namespace archive
      {
         namespace json
         {
            archive::Reader reader( const std::string& destination);
            archive::Reader reader( std::istream& destination);
            archive::Reader reader( const common::platform::binary::type& destination);

            namespace relaxed
            {    
               archive::Reader reader( const std::string& destination);
               archive::Reader reader( std::istream& destination);
               archive::Reader reader( const common::platform::binary::type& destination);
            }

            namespace consumed
            {    
               archive::Reader reader( const std::string& source);
               archive::Reader reader( std::istream& source);
               archive::Reader reader( const common::platform::binary::type& source);
            }

            archive::Writer writer( std::string& destination);
            archive::Writer writer( std::ostream& destination);
            archive::Writer writer( common::platform::binary::type& destination);

         } // json
      } // archive
   } // serviceframework
} // casual




#endif /* ARCHIVE_JSAON_H_ */
