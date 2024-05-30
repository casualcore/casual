//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/message/protocol.h"

#include "common/algorithm.h"


#include <ostream>

namespace casual
{
   namespace gateway::message::protocol
   {

      
      std::string_view description( Version value) noexcept
      {
         switch( value)
         {
            case Version::invalid: return "invalid";
            case Version::v1_0: return "v1.0";
            case Version::v1_1: return "v1.1";
            case Version::v1_2: return "v1.2";
            case Version::v1_3: return "v1.3";
         };
         return "<unknown>";
      }

   } //gateway::message::protocol
} // casual