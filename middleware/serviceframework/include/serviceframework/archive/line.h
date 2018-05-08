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




