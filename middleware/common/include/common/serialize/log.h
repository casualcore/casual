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
            //! @returns a 'writer' that serialize to a
            //! _yaml-like_ format that is (subjectively) easy 
            // to read for a human
            Writer writer();

         } // log
      } // serialize
   } // common
} // casual




