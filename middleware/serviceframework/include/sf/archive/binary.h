//!
//! casual
//!

#ifndef ARCHIVE_BINARY_H_
#define ARCHIVE_BINARY_H_


#include "sf/archive/archive.h"
#include "common/platform.h"

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace binary
         {


            archive::Reader reader( const common::platform::binary::type& destination);

            archive::Writer writer( common::platform::binary::type& destination);

         } // binary
      } // archive
   } // sf
} // casual

#endif /* ARCHIVE_BINARY_H_ */
