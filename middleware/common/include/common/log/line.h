//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/log/stream.h"
#include "common/log/category.h"

namespace casual
{
   namespace common::log
   {
      namespace detail
      {
         template< typename T>
         concept code_like = std::is_error_code_enum_v< T> || std::is_error_condition_enum_v< T>;
      } // detail

      template< typename... Args>
      void line( std::ostream& out, Args&&... args)
      {
         log::write( out, std::forward< Args>( args)..., '\n');
      }

      template< detail::code_like Code, typename... Args>
      void code( std::ostream& out, Code code, Args&&... args)
      {
         if constexpr( sizeof...( Args) == 0)
            log::line( out, code);
         else
            log::line( out, code, " ", std::forward< Args>( args)...);
      }


      template< detail::code_like Code, typename... Args>
      auto error( Code code, Args&&... args)
      {
         log::code( log::category::error, code, std::forward< Args>( args)...);
      }

      namespace verbose
      {
         template< detail::code_like Code, typename... Args>
         auto error( Code code, Args&&... args)
         {
            log::code( log::category::verbose::error, code, std::forward< Args>( args)...);
         }
      } // verbose

      template< detail::code_like Code, typename... Args>
      auto warning( Code code, Args&&... args)
      {
         log::code( log::category::warning, code, std::forward< Args>( args)...);
      }
      
   } // common::log
   
} // casual
