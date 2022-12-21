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

#include "casual/assert.h"

#include <map>
#include <ostream>

namespace casual
{
   namespace common::service
   {
      namespace visibility
      {
         std::string_view description( Type value) noexcept
         {
            switch( value)
            {
               case Type::discoverable: return "discoverable";
               case Type::undiscoverable: return "undiscoverable";
            }
            return "<unknown>";
         }

         Type transform( std::string_view value)
         {
            if( value == "discoverable") return Type::discoverable;
            if( value == "undiscoverable") return Type::undiscoverable;
            
            code::raise::error( code::casual::invalid_configuration, "unexpected value: ", value);
         }

         std::string transform( Type value)
         {
            return std::string{ description( value)};
         }

         Type transform( short value)
         {
            CASUAL_ASSERT( value >= cast::underlying( Type::discoverable) && value <= cast::underlying( Type::undiscoverable));

            return static_cast< Type>( value);
         }
         
      } // visibility

      namespace execution::timeout::contract
      {
         Type transform( std::string_view contract)
         {
            if( contract == "linger") return Type::linger;
            if( contract == "kill") return Type::kill;
            if( contract == "terminate") return Type::terminate;
            
            code::raise::error( code::casual::invalid_configuration, "unexpected value: ", contract);
         }

         std::string transform( Type value)
         {
            return std::string{ description( value)};
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
               case Type::automatic: return "auto";
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
