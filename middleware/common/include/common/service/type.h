//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/code/xatmi.h"
#include "common/serialize/macro.h"

#include <string>
#include <iosfwd>

#include <cstdint>

namespace casual
{
   namespace common::service
   {
      struct Code 
      {
         code::xatmi result = code::xatmi::ok;
         long user{};

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( result);
            CASUAL_SERIALIZE( user);
         )
      };

      namespace category
      {
         constexpr std::string_view none = "";
         constexpr std::string_view admin = ".admin";
         constexpr std::string_view deprecated = ".deprecated";
      } // category

      namespace hidden
      {
         //! @returns true if the service `name` is _hidden_ (starts with `.`)
         bool name( std::string_view service);         
      } // hidden

      namespace visibility
      {
         enum class Type : short
         {
            discoverable = 1,
            undiscoverable
         };
         
         std::string_view description( Type value) noexcept;

         Type transform( std::string_view contract);
         std::string transform( Type contract);
         Type transform( short mode);
         
      } // visibility

      namespace execution::timeout::contract
      {
         enum class Type : short
         {
            linger,
            interrupt,
            kill,
            abort
         };
         
         std::string_view description( Type value) noexcept;

         Type transform( std::string_view contract);
         std::string transform( Type contract);

         //! @returns true if timeout contract is fatal
         bool fatal( Type value) noexcept;
      }

      namespace transaction
      {
         enum class Type : short
         {
            //! join transaction if present else start a new transaction
            automatic = 0,
            //! join transaction if present else execute outside transaction
            join = 1,
            //! start a new transaction regardless
            atomic = 2,
            //! execute outside transaction regardless
            none = 3,
            //! branch transaction if present, else start a new transaction
            branch,
         };

         std::string_view description( Type value) noexcept;

         Type mode( std::string_view mode);
         Type mode( std::uint16_t mode);

      } // transaction
   } // common::service
} // casual


