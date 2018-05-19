//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#pragma once


#include "serviceframework/archive/archive.h"
#include <iosfwd>

namespace casual
{
   namespace serviceframework
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
   } // serviceframework
} // casual




