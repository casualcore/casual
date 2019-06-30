//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#pragma once


#include "common/serialize/archive.h"
#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace serialize
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
      } // serialize
   } // common
} // casual




