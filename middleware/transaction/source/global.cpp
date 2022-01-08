//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "transaction/global.h"

#include "common/transcode.h"

#include "casual/assert.h"

#include <ostream>

namespace casual
{
   namespace transaction::global
   {
      ID::ID( const common::transaction::ID& trid)
      {
         auto global = common::transaction::id::range::global( trid);
         assertion( global.size() <= 64, "trid: ", trid, " has larger gtrid size than 64");

         m_size = global.size();
         common::algorithm::copy( global, std::begin( m_gtrid));
      }

      std::ostream& operator << ( std::ostream& out, const ID& value)
      {
         return common::transcode::hex::encode( out, value());
      }

   } // transaction::global
} // casual