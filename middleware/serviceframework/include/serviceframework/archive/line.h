//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#ifndef CASUAL_SF_ARCHIVE_LINE_H_
#define CASUAL_SF_ARCHIVE_LINE_H_

#include "serviceframework/archive/archive.h"
#include <iosfwd>

namespace casual
{
   namespace serviceframework
   {

      namespace archive
      {
         namespace line
         {

            //!
            //! serialize serializable objects to one line json-like format
            //!
            Writer writer( std::ostream& out);

         } // log
      } // archive
   } // serviceframework
} // casual



#endif /* CASUAL_SF_ARCHIVE_LINE_H_ */
