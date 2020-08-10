//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/code/log.h"

#include "common/log/category.h"
#include "common/log.h"

#include <system_error>


namespace casual
{
   namespace common
   {
      namespace code
      {
         namespace raise
         {
            template< typename Code>
            [[noreturn]] void condition( Code code) noexcept( false)
            {
               throw std::error_code{ code};
            }

            template< typename Code>
            [[noreturn]] void generic( Code code, std::ostream& category) noexcept( false)
            {
               common::log::line( category, code);
               raise::condition( code);
            }

            template< typename Code, typename... Ts>
            [[noreturn]] void generic( Code code, std::ostream& category, Ts&&... ts) noexcept( false)
            {
               common::log::line( category, code, ' ', std::forward< Ts>( ts)...);
               raise::condition( code);
            }
            
            //! streams to error and throws `code`
            template< typename Code, typename... Ts>
            [[noreturn]] void error( Code code, Ts&&... ts) noexcept( false)
            {
               raise::generic( code, common::log::category::error, std::forward< Ts>( ts)...);
            }

            //! streams to debug and throws `code`
            template< typename Code, typename... Ts>
            [[noreturn]] void log( Code code, Ts&&... ts) noexcept( false)
            {
               
               raise::generic( code, code::stream( code), std::forward< Ts>( ts)...);
            }
         } // raise
      } // code
   } // common
} // casual