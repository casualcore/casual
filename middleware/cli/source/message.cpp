//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/cli/message.h"

#include "common/algorithm.h"

namespace casual
{
   namespace cli::message
   {
      
      namespace pipe
      {
         std::string_view description( State value) noexcept
         {
            switch( value)
            {
               case State::ok: return "ok";
               case State::error: return "error";
            }
            return "<unknown>";
         }
         
      } // pipe

      namespace to
      {
         void human< queue::message::ID>::stream( const queue::message::ID& message)
         {
            std::cout << message.id << '\n';
            std::cout.flush();
         }
      } // to

   } // cli::message
} // casual