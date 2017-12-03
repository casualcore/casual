//!
//! casual
//!

#ifndef CASUAL_ARCHIVE_YAML_POLICY_H_
#define CASUAL_ARCHIVE_YAML_POLICY_H_

#include "sf/archive/archive.h"
#include "sf/platform.h"


#include <iosfwd>
#include <string>

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace yaml
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

            archive::Writer writer( std::string& destination);
            archive::Writer writer( std::ostream& destination);
            archive::Writer writer( common::platform::binary::type& destination);

         } // yaml
      } // archive
   } // sf
} // casual



#endif /* CASUAL_ARCHIVE_YAML_POLICY_H_ */
