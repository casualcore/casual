//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/cli/message.h"

#include "common/algorithm.h"

namespace casual
{
   namespace cli
   {
      namespace message
      {
         namespace to
         {
            void human< queue::message::ID>::stream( const queue::message::ID& message)
            {
               std::cout << message.id << '\n';
               std::cout.flush();
            }


         } // to
      } // message
   } // cli
} // casual