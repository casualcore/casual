//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_ARCHIVE_YAML_POLICY_H_
#define CASUAL_ARCHIVE_YAML_POLICY_H_

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
         namespace yaml
         {


            archive::Reader reader( const std::string& source);
            archive::Reader reader( std::istream& source);
            archive::Reader reader( const common::platform::binary::type& source);

            namespace relaxed
            {    
               archive::Reader reader( const std::string& source);
               archive::Reader reader( std::istream& source);
               archive::Reader reader( const common::platform::binary::type& source);
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

         } // yaml
      } // archive
   } // serviceframework
} // casual



#endif /* CASUAL_ARCHIVE_YAML_POLICY_H_ */
