//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_TYPE_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_TYPE_H_


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
            constexpr auto none() { return "";}
            constexpr auto admin() { return ".admin";}
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
               none = 3
            };

            std::ostream& operator << ( std::ostream& out, Type value);

            Type mode( const std::string& mode);
            Type mode( std::uint16_t mode);

         } // transaction

      } // service

   } // common


} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_TYPE_H_
