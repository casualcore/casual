//!
//! casual
//!

#ifndef CASUAL_SF_ARCHIVE_MAKER_H_
#define CASUAL_SF_ARCHIVE_MAKER_H_


#include "sf/archive/archive.h"

#include "sf/platform.h"

#include <string>
#include <iosfwd>


namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace reader
         {
            namespace from
            {
               archive::Reader file( const std::string& name);

               //! @{
               archive::Reader data();
               archive::Reader data( std::istream& stream);
               //! @}

               //! @{
               archive::Reader name( std::string name);
               archive::Reader name( std::istream& stream, std::string name);
               //! @}

               archive::Reader buffer( const platform::binary::type& data, std::string name);
            } // from
         } // reader


         namespace writer
         {
            namespace from
            {
               //! @{
               archive::Writer name( std::string name);
               archive::Writer name( std::ostream& stream, std::string type);
               //! @}

               archive::Writer buffer( platform::binary::type& data, std::string type);
            } // from
         } // writer
      } // archive
   } // sf
} // casual

#endif /* CASUAL_SF_ARCHIVE_MAKER_H_ */
