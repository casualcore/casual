//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/service/type.h"
#include "common/cast.h"
#include "common/string.h"

#include "common/code/raise.h"
#include "common/code/casual.h"


#include <map>
#include <ostream>

namespace casual
{
   namespace common::service
   {
      namespace execution::timeout::contract
      {
         Type transform( std::string_view contract)
         {
            if( contract == "linger") return Type::linger;
            if( contract == "kill") return Type::kill;
            if( contract == "terminate") return Type::terminate;
            
            code::raise::error( code::casual::invalid_configuration, "unexpected value: ", contract);
         }

         std::string transform( Type contract)
         {
            std::ostringstream stream{};
            stream::write( stream, contract);
            return std::move( stream).str();
         }

         std::string_view description( Type value) noexcept
         {
            switch( value)
            {
               case Type::linger: return "linger";
               case Type::kill: return "kill";
               case Type::terminate: return "terminate";
            }
            return "unknown";
         }

      }

      namespace transaction
      {
         std::string_view description( Type value) noexcept
         {
            switch( value)
            {
               case Type::atomic: return "atomic";
               case Type::join: return "join";
               case Type::automatic: return "automatic";
               case Type::none: return "none";
               case Type::branch: return "branch";
            }
            return "unknown";
         }

         Type mode( std::string_view mode)
         {
            if( mode == "automatic" || mode == "auto") return Type::automatic;
            if( mode == "join") return Type::join;
            if( mode == "atomic") return Type::atomic;
            if( mode == "none") return Type::none;
            if( mode == "branch") return Type::branch;

            code::raise::error( code::casual::invalid_argument, "transaction mode: ", mode);
         }

         Type mode( std::uint16_t mode)
         {
            if( mode < cast::underlying( Type::automatic) || mode > cast::underlying( Type::branch))
               code::raise::error( code::casual::invalid_argument, "transaction mode: ", mode);

            return static_cast< Type>( mode);
         }

      } // transaction
   } // common::service
} // casual
