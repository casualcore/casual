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

      std::ostream& operator << ( std::ostream& out, Version value)
      {
         if( ! common::algorithm::find( versions, value))
            return out << "invalid";

         return out << std::to_underlying( value) / 1000 << '.' << std::to_underlying( value) % 1000;
      }

   } //gateway::message::protocol
} // casual