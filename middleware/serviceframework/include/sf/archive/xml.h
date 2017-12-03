//!
//! casual
//!

#ifndef CASUAL_SF_ARCHIVE_XML_H_
#define CASUAL_SF_ARCHIVE_XML_H_

#include "sf/archive/archive.h"
#include "sf/platform.h"

#include <iosfwd>
#include <string>
#include <vector>


namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace xml
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
         } // xml
      } // archive
   } // sf
} // casual



#endif /* CASUAL_SF_ARCHIVE_XML_H_ */
