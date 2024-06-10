//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/message/protocol.h"

#include "common/algorithm/is.h"


#include <ostream>

namespace casual
{
   namespace gateway::message::protocol
   {
      // make sure the versions are sorted in reverse order.
      static_assert( common::algorithm::is::sorted( common::range::reverse( protocol::versions)));

   } //gateway::message::protocol
} // casual