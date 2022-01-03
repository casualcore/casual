//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "transaction/global.h"

#include "common/transcode.h"

#include <ostream>

namespace casual
{
   namespace transaction::global
   {

      std::ostream& operator << ( std::ostream& out, const ID& value)
      {
         return common::transcode::hex::encode( out, value.global());
      }

   } // transaction::global
} // casual