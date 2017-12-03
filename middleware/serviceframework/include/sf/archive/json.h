//!
//! casual
//!

#ifndef ARCHIVE_JSON_H_
#define ARCHIVE_JSON_H_


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

            archive::Writer writer( std::string& destination);
            archive::Writer writer( std::ostream& destination);
            archive::Writer writer( common::platform::binary::type& destination);

         } // json
      } // archive
   } // sf
} // casual




#endif /* ARCHIVE_JSAON_H_ */
