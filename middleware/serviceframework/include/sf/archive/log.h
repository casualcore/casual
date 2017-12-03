//!
//! casual
//!


#ifndef CASUAL_SF_ARCHIVE_LOG_H_
#define CASUAL_SF_ARCHIVE_LOG_H_

#include "sf/archive/archive.h"
#include <iosfwd>

namespace casual
{
   namespace sf
   {

      namespace archive
      {
         namespace log
         {

            //!
            //! |-someStuff
            //! ||-name...........[blabla]
            //! ||-someOtherName..[foo]
            //! ||-composite
            //! |||-foo..[slkjf]
            //! |||-bar..[42]
            //! ||-
            //!
            Writer writer( std::ostream& out);

         } // log
      } // archive
   } // sf
} // casual



#endif /* ARCHIVE_LOGGER_H_ */
