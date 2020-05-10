//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include <string>
#include <iosfwd>

#include <cstdint>

namespace casual
{
   namespace common
   {
      namespace service
      {
         namespace category
         {
            constexpr auto none = "";
            constexpr auto admin = ".admin";
            constexpr auto deprecated = ".deprecated";
         } // category

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

            std::ostream& operator << ( std::ostream& out, Type value);

            Type mode( const std::string& mode);
            Type mode( std::uint16_t mode);

         } // transaction
      } // service
   } // common
} // casual


